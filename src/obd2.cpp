#include "obd2.h"
#include "can/canutil.h"
#include "util/timer.h"
#include <limits.h>

namespace time = openxc::util::time;

#define ENGINE_SPEED_PID 0xc
#define VEHICLE_SPEED_PID 0xd

static bool ENGINE_STARTED = false;
static bool VEHICLE_IN_MOTION = false;

static openxc::util::time::FrequencyClock IGNITION_STATUS_TIMER = {0.2};

// TODO we're kind of abusing the value decoder to change some state - would it
// be better to have an explicit callback entry on the ActiveDiagnosticRequest?
// it could provide the request as a parameter and the fully decoded value. I
// like that better even if it does add 4 bytes (another pointer) to the struct.
static float checkIgnitionStatus(
        const DiagnosticResponse* response, float parsedPayload) {
    float value = diagnostic_decode_obd2_pid(response);
    bool match = false;
    if(response->pid == ENGINE_SPEED_PID) {
        match = true;
        ENGINE_STARTED = value != 0;
    } else if(response->pid == VEHICLE_SPEED_PID) {
        VEHICLE_IN_MOTION = value != 0;
        match = true;
    }

    if(match) {
        time::tick(&IGNITION_STATUS_TIMER);
    }
    return value;
}

static float checkSupportedPids(const DiagnosticResponse* response,
        float parsedPayload) {
    for(int i = 0; i < response->payload_length; i++) {
        for(int j = CHAR_BIT - 1; j >= 0; j--) {
            if(response->payload[i] >> j & 0x1) {
                // uint16_t pid = response->pid + (i * CHAR_BIT) + j + 1;
                // DiagnosticRequest request = {arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
                        // mode: 0x1, has_pid: true, pid: pid};
                // TODO need a function with no factor/offset, just a decoder
                // TODO where to get manager and bus?
                // addDiagnosticRequest(manager, bus, &request, "obd2_pid",
                        // 1, 0, handleObd2Pid, 1);
                // TODO lookup the predefined names for these
                // TODO lookup the predefined frequencies for these
                // TODO configure to only send when listener is attached, except
                // don't want to clobber RPM/veh speed for power check
            }
        }
    }
    return 0;
}

static void createRecurringIgnitionStatusCheck(
        openxc::diagnostics::DiagnosticsManager* manager, CanBus* bus) {
    // TODO how do we protect these? don't want to be able to delete them,
    // otherwise we could lose track of ignition we also don't want to blow away
    // an existing request at a faster frequency. may need to update API in
    // ::diagnostics. a simple 'force update' flag? but we *do* want to force
    // update if someone else set it to less than 1hz. this exposes a weakness
    // in the update/delete API, since there's no unique ID assigned to each
    // request you add. probably overthinking it.
    DiagnosticRequest request = {arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
            mode: 0x1, has_pid: true, pid: ENGINE_SPEED_PID};
    addDiagnosticRequest(manager, bus, &request, "engine_speed", 1, 0, checkIgnitionStatus, 1);

    request.pid = VEHICLE_SPEED_PID;
    addDiagnosticRequest(manager, bus, &request, "vehicle_speed", 1, 0, checkIgnitionStatus, 1);
}

void openxc::diagnostics::obd2::initialize(DiagnosticsManager* manager, CanBus* bus) {
    createRecurringIgnitionStatusCheck(manager, bus);
    time::tick(&IGNITION_STATUS_TIMER);
}

// * CAN traffic will eventualy stop, and we will suspend.
// * When do we wake up?
// * If normal CAN is open, bus activity will wake us up and we will resume.
// * If normal CAN is blocked, we rely on a watchdog to wake us up every 15 seconds to
//      start this process over again.
void openxc::diagnostics::obd2::loop(DiagnosticsManager* manager, CanBus* bus) {
    static bool ignitionWasOn = false;
    static bool pidSupportQueried = false;

    if((ignitionWasOn && !ENGINE_STARTED && !VEHICLE_IN_MOTION) ||
            time::elapsed(&IGNITION_STATUS_TIMER, false)) {
        // remove all open diagnostic requests, which shuld cause the bus to go
        // silent if the car is off, and thus the VI to suspend. TODO kick off
        // watchdog!
        diagnostics::reset(manager);
        ignitionWasOn = false;
        pidSupportQueried = false;
    } else if(ENGINE_STARTED || VEHICLE_IN_MOTION) {
        ignitionWasOn = true;
        // TODO check a flag to decide if the user wants the OBD-II set enabled
        if(!pidSupportQueried) {
            pidSupportQueried = true;
            DiagnosticRequest request = {arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
                    mode: 0x1, has_pid: true, pid: 0x0};
            for(int i = 0x0; i <= 0x80; i += 0x20) {
                request.pid = i;
                addDiagnosticRequest(manager, bus, &request, NULL, 1, 0, checkSupportedPids, 0);
            }
        }
    }
}
