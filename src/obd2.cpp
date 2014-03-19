#include "obd2.h"
#include "can/canutil.h"
#include "util/timer.h"
#include "util/log.h"
#include "shared_handlers.h"
#include "config.h"
#include <limits.h>

namespace time = openxc::util::time;

using openxc::diagnostics::DiagnosticsManager;
using openxc::util::log::debug;
using openxc::diagnostics::ActiveDiagnosticRequest;
using openxc::config::getConfiguration;

#define ENGINE_SPEED_PID 0xc
#define VEHICLE_SPEED_PID 0xd

static bool ENGINE_STARTED = false;
static bool VEHICLE_IN_MOTION = false;

static openxc::util::time::FrequencyClock IGNITION_STATUS_TIMER = {0.2};

static void checkIgnitionStatus(DiagnosticsManager* manager,
        const ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response,
        float parsedPayload) {
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
}

static void checkSupportedPids(DiagnosticsManager* manager,
        const ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response,
        float parsedPayload) {
    if(manager->obd2Bus == NULL || getConfiguration()->recurringObd2Requests) {
        return;
    }

    for(int i = 0; i < response->payload_length; i++) {
        for(int j = CHAR_BIT - 1; j >= 0; j--) {
            if(response->payload[i] >> j & 0x1) {
                uint16_t pid = response->pid + (i * CHAR_BIT) + j + 1;
                DiagnosticRequest request = {
                        arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
                        mode: 0x1, has_pid: true, pid: pid};
                addRecurringRequest(manager, manager->obd2Bus, &request, "obd2_pid", false,
                        1, 0, openxc::signals::handlers::handleObd2Pid, NULL, 1, false);
                // TODO lookup the predefined names for these
                // TODO lookup the predefined frequencies for these
                // TODO configure to only send when listener is attached, except
                // don't want to clobber RPM/veh speed for power check
            }
        }
    }
}

static void requestIgnitionStatus(DiagnosticsManager* manager) {
    if(manager->obd2Bus != NULL) {
        DiagnosticRequest request = {arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
                mode: 0x1, has_pid: true, pid: ENGINE_SPEED_PID};
        addRecurringRequest(manager, manager->obd2Bus, &request, "engine_speed",
                false, 1, 0, NULL, checkIgnitionStatus, 0, false);

        request.pid = VEHICLE_SPEED_PID;
        addRecurringRequest(manager, manager->obd2Bus, &request, "vehicle_speed",
                false, 1, 0, NULL, checkIgnitionStatus, 0, false);
        time::tick(&IGNITION_STATUS_TIMER);
    }
}

void openxc::diagnostics::obd2::initialize(DiagnosticsManager* manager) {
    if(manager->obd2Bus != NULL) {
        requestIgnitionStatus(manager);
    } else {
        debug("No bus configured for OBD2 queries, not enabling");
    }
}

// * CAN traffic will eventualy stop, and we will suspend.
// * When do we wake up?
// * If normal CAN is open, bus activity will wake us up and we will resume.
// * If normal CAN is blocked, we rely on a watchdog to wake us up every 15
// seconds to start this process over again.
void openxc::diagnostics::obd2::loop(DiagnosticsManager* manager, CanBus* bus) {
    static bool ignitionWasOn = false;
    static bool pidSupportQueried = false;
    static bool sentFinalIgnitionCheck = false;

    if((ignitionWasOn && !ENGINE_STARTED && !VEHICLE_IN_MOTION) ||
            (sentFinalIgnitionCheck && time::elapsed(&IGNITION_STATUS_TIMER, false))) {
        // remove all open diagnostic requests, which shuld cause the bus to go
        // silent if the car is off, and thus the VI to suspend. TODO kick off
        // watchdog! TODO when it wakes keep in a minimum run level (i.e. don't
        // turn on bluetooth) until we decide the vehicle is actually on.
        diagnostics::reset(manager);
        ignitionWasOn = false;
        pidSupportQueried = false;
    } else if(time::elapsed(&IGNITION_STATUS_TIMER, false)) {
        // We haven't received an ignition in 5 seconds. Either the user didn't
        // have either OBD-II request configured as a recurring request (which
        // is fine) or they did, but the car stopped responding. Kick off
        // another request to see which is true. It will take 5+5 seconds after
        // ignition off to decide we should shut down.
        requestIgnitionStatus(manager);
        time::tick(&IGNITION_STATUS_TIMER);
        sentFinalIgnitionCheck = true;
    } else if(ENGINE_STARTED || VEHICLE_IN_MOTION) {
        ignitionWasOn = true;
        sentFinalIgnitionCheck = false;
        if(getConfiguration()->recurringObd2Requests && !pidSupportQueried) {
            pidSupportQueried = true;
            DiagnosticRequest request = {
                    arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
                    mode: 0x1,
                    has_pid: true,
                    pid: 0x0};
            for(int i = 0x0; i <= 0x80; i += 0x20) {
                request.pid = i;
                addRecurringRequest(manager, bus, &request, NULL, false, 1, 0,
                        NULL, checkSupportedPids, 0, false);
            }
        }
    }
}
