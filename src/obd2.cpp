#include "obd2.h"
#include "can/canutil.h"
#include "util/timer.h"
#include "util/log.h"
#include "shared_handlers.h"
#include "config.h"
#include <limits.h>

namespace time = openxc::util::time;

using openxc::util::log::debug;
using openxc::diagnostics::DiagnosticsManager;
using openxc::diagnostics::ActiveDiagnosticRequest;
using openxc::config::getConfiguration;
using openxc::config::PowerManagement;
using openxc::config::RunLevel;

#define ENGINE_SPEED_PID 0xc
#define VEHICLE_SPEED_PID 0xd

static bool ENGINE_STARTED = false;
static bool VEHICLE_IN_MOTION = false;

static openxc::util::time::FrequencyClock IGNITION_STATUS_TIMER = {0.5, 0, NULL};

/* Private: A representation of an OBD-II PID.
 *
 * pid - The 1 byte PID.
 * name - A human readable name to use for this PID when published.
 * frequency - The frequency to request this PID if supported by the vehicle
 *      when automatic, recurring OBD-II requests are enabled.
 */
typedef struct {
    uint8_t pid;
    const char* name;
    float frequency;
} Obd2Pid;

/* Private: Pre-defined OBD-II PIDs to query for if supported by the vehicle.
 */
const Obd2Pid OBD2_PIDS[] = {
    { pid: ENGINE_SPEED_PID, name: "engine_speed", frequency: 5 },
    { pid: VEHICLE_SPEED_PID, name: "vehicle_speed", frequency: 5 },
    { pid: 0x4, name: "engine_load", frequency: 5 },
    { pid: 0x5, name: "engine_coolant_temperature", frequency: 1 },
    { pid: 0x33, name: "barometric_pressure", frequency: 1 },
    { pid: 0x4c, name: "commanded_throttle_position", frequency: 1 },
    { pid: 0x27, name: "fuel_level", frequency: 1 },
    { pid: 0xf, name: "intake_air_temperature", frequency: 1 },
    { pid: 0xb, name: "intake_manifold_pressure", frequency: 1 },
    { pid: 0x1f, name: "running_time", frequency: 1 },
    { pid: 0x11, name: "throttle_position", frequency: 5 },
    { pid: 0xa, name: "fuel_pressure", frequency: 1 },
    { pid: 0x10, name: "mass_airflow", frequency: 5 },
    { pid: 0x5a, name: "accelerator_pedal_position", frequency: 5 },
    { pid: 0x52, name: "ethanol_fuel_percentage", frequency: 1 },
    { pid: 0x5c, name: "engine_oil_temperature", frequency: 1 },
    { pid: 0x63, name: "engine_torque", frequency: 1 },
};

static void checkIgnitionStatus(DiagnosticsManager* manager,
        const ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response,
        float parsedPayload) {
    bool match = false;
    if(response->pid == ENGINE_SPEED_PID) {
        match = ENGINE_STARTED = parsedPayload != 0;
    } else if(response->pid == VEHICLE_SPEED_PID) {
        match = VEHICLE_IN_MOTION = parsedPayload != 0;
    }

    if(match) {
        time::tick(&IGNITION_STATUS_TIMER);
    }
}

static void requestIgnitionStatus(DiagnosticsManager* manager) {
    if(manager->obd2Bus != NULL && (getConfiguration()->powerManagement ==
                PowerManagement::OBD2_IGNITION_CHECK ||
            getConfiguration()->recurringObd2Requests)) {
        debug("Sending requests to check ignition status");
        DiagnosticRequest request = {arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
                mode: 0x1, has_pid: true, pid: ENGINE_SPEED_PID};
        addRequest(manager, manager->obd2Bus, &request, "engine_speed", false,
                NULL, checkIgnitionStatus);

        request.pid = VEHICLE_SPEED_PID;
        addRequest(manager, manager->obd2Bus, &request, "vehicle_speed", false,
                NULL, checkIgnitionStatus);
        time::tick(&IGNITION_STATUS_TIMER);
    }
}

static void checkSupportedPids(DiagnosticsManager* manager,
        const ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response,
        float parsedPayload) {
    if(manager->obd2Bus == NULL || !getConfiguration()->recurringObd2Requests) {
        return;
    }

    debug("%s", "Querying for supported PIDs from vehicle");
    for(int i = 0; i < response->payload_length; i++) {
        for(int j = CHAR_BIT - 1; j >= 0; j--) {
            if(response->payload[i] >> j & 0x1) {
                uint16_t pid = response->pid + (i * CHAR_BIT) + j + 1;
                debug("Vehicle supports PID 0x%02x", pid);
                for(size_t i = 0; i < sizeof(OBD2_PIDS) / sizeof(Obd2Pid); i++) {
                    if(OBD2_PIDS[i].pid == pid) {
                        debug("Automatically adding recurring request for PID 0x%x", pid);
                        DiagnosticRequest request = {
                                arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
                                mode: 0x1, has_pid: true, pid: pid};
                        addRecurringRequest(manager, manager->obd2Bus, &request,
                                OBD2_PIDS[i].name, false,
                                openxc::diagnostics::obd2::handleObd2Pid,
                                checkIgnitionStatus, OBD2_PIDS[i].frequency);
                        break;
                    }
                }
            }
        }
    }
}

void openxc::diagnostics::obd2::initialize(DiagnosticsManager* manager) {
    requestIgnitionStatus(manager);
}

// * CAN traffic will eventualy stop, and we will suspend.
// * When do we wake up?
// * If normal CAN is open, bus activity will wake us up and we will resume.
// * If normal CAN is blocked, we rely on a watchdog to wake us up every 15
// seconds to start this process over again.
void openxc::diagnostics::obd2::loop(DiagnosticsManager* manager) {
    static bool pidSupportQueried = false;
    const int MAX_IGNITION_CHECK_COUNT = 3;
    static int ignitionCheckCount = 0;

    if(!manager->initialized || manager->obd2Bus == NULL) {
        return;
    }

    if(time::elapsed(&IGNITION_STATUS_TIMER, false)) {
        if(ignitionCheckCount >= MAX_IGNITION_CHECK_COUNT &&
                getConfiguration()->powerManagement ==
                        PowerManagement::OBD2_IGNITION_CHECK) {
            debug("Ignition appears to be off - reducing frequency of ignition checks");
            diagnostics::reset(manager);
            // Don't stop sending requests altogether, because if the CAN bus is still
            // active we want to keep querying for ignition. If we stop sending
            // ignition checks here we risk getting stuck awake, but not querying
            // for any diagnostics messages. Additionally the time between ignition checks
            // must be larger than the CAN_ACTIVE_TIMEOUT, otherwise we'll never suspend
            // even if the CAN bus goes inactive.
            IGNITION_STATUS_TIMER.frequency = 1.0 / (openxc::can::CAN_ACTIVE_TIMEOUT_S + 5);
            ignitionCheckCount = 0;
            pidSupportQueried = false;
        } else {
            // We haven't received an ignition in 5 seconds. Either the user didn't
            // have either OBD-II request configured as a recurring request (which
            // is fine) or they did, but the car stopped responding. Kick off
            // another request to see which is true. It will take 5+5 seconds after
            // ignition off to decide we should cancel all outstanding requests.
            requestIgnitionStatus(manager);
            ++ignitionCheckCount;
        }
    } else if(ENGINE_STARTED || VEHICLE_IN_MOTION) {
        IGNITION_STATUS_TIMER.frequency = 0.5;
        ignitionCheckCount = 0;
        getConfiguration()->desiredRunLevel = RunLevel::ALL_IO;
        if(getConfiguration()->recurringObd2Requests && !pidSupportQueried) {
            debug("Ignition is on - querying for supported OBD-II PIDs");
            pidSupportQueried = true;
            DiagnosticRequest request = {
                    arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
                    mode: 0x1,
                    has_pid: true,
                    pid: 0x0};
            for(int i = 0x0; i <= 0x80; i += 0x20) {
                request.pid = i;
                addRequest(manager, manager->obd2Bus, &request, NULL, false,
                        NULL, checkSupportedPids);
            }
        }
    }
}

bool openxc::diagnostics::obd2::isObd2Request(DiagnosticRequest* request) {
    return request->mode == 0x1 && request->pid < 0xff;
}

void openxc::diagnostics::obd2::handleObd2Pid(
        const DiagnosticResponse* response, float parsedPayload, char* str_buf, int buf_size) {
    snprintf(str_buf, buf_size, "%f", diagnostic_decode_obd2_pid(response));
}
