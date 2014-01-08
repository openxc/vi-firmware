#include "diagnostics.h"
#include "signals.h"
#include "can/canwrite.h"
#include "util/log.h"
#include <bitfield/bitfield.h>
#include <limits.h>

using openxc::diagnostics::DiagnosticsManager;
using openxc::signals::getCanBuses;
using openxc::util::log::debug;

DiagnosticRequestHandle DIAG_HANDLE;

static bool sendDiagnosticCanMessage(const uint16_t arbitration_id,
        const uint8_t* data, const uint8_t size) {
    // TODO right now this doens't support sending requests on anything but bus
    // 1. a few things need to change to support uses bus 2, or using different
    // buses for different requests.
    //
    // this is OK to use the signals.h functions directly here, since we intend
    // for those to change at runtime soon - we don't want to provide the buses
    // when the DiagnosticsManager is initialized, for example.
    //
    // second, the send_can_message shim prototype from isotp-c doesn't have an
    // argument to identify the bus for the message, so we need to add that
    const CanBus* bus = &getCanBuses()[0];
    const CanMessage message = {arbitration_id, get_bitfield(data, size, 0,
            size * CHAR_BIT)};
    return openxc::can::write::sendCanMessage(bus, &message);
}

void openxc::diagnostics::initialize(DiagnosticsManager* manager) {
    manager->shims = diagnostic_init_shims(openxc::util::log::debug,
                       sendDiagnosticCanMessage, NULL);
}

// TODO when deciding to send requests
//     is last request completed, or is it a broadcast request (and thus
//     doesn't ever complete?)
//      if yes, is the clock ticked to request again?
//
// when adding a new diag request, if the list is full
//  find first broadcast, non-recurring request and delete it - keeps single use
//  braodcast around as long as possible to collect responses

void openxc::diagnostics::sendRequests(DiagnosticsManager* manager) {
    if(DIAG_HANDLE.completed) {
        DIAG_HANDLE = diagnostic_request_pid(&manager->shims,
            DIAGNOSTIC_STANDARD_PID, 0x7df, 0x11, NULL);
    }
}

void openxc::diagnostics::receiveCanMessage(DiagnosticsManager* manager,
        CanMessage* message) {
    if(!DIAG_HANDLE.completed) {
        // TODO could we put this union in CanMessage? may not be necessary
        // if we migrate way from uint64_t but i don't think it would
        // negatively impact memory
        ArrayOrBytes combined;
        combined.whole = message->data;
        DiagnosticResponse response = diagnostic_receive_can_frame(&manager->shims,
                &DIAG_HANDLE, message->id, combined.bytes,
                sizeof(combined.bytes));
        if(response.completed && DIAG_HANDLE.completed) {
            if(DIAG_HANDLE.success) {
                if(response.success) {
                    debug("Diagnostic response received: arb_id: 0x%02x, mode: 0x%x, pid: 0x%x, payload: 0x%02x%02x%02x%02x%02x%02x%02x%02x, size: %d",
                            response.arbitration_id,
                            response.mode,
                            response.pid,
                            response.payload[0],
                            response.payload[1],
                            response.payload[2],
                            response.payload[3],
                            response.payload[4],
                            response.payload[5],
                            response.payload[6],
                            response.payload[7],
                            response.payload_length);

                } else {
                    debug("Negative diagnostic response received, NRC: 0x%x",
                            response.negative_response_code);
                }
            } else {
                debug("Fatal error wen sending or receiving diagnostic request");
            }
        }
    }
}
