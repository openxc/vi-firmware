#include "diagnostics.h"
#include "signals.h"
#include "can/canwrite.h"
#include "can/canread.h"
#include "util/log.h"
#include "util/timer.h"
#include <bitfield/bitfield.h>
#include <limits.h>

using openxc::diagnostics::ActiveRequestList;
using openxc::diagnostics::ActiveDiagnosticRequest;
using openxc::diagnostics::ActiveRequestListEntry;
using openxc::diagnostics::DiagnosticsManager;
using openxc::util::log::debug;
using openxc::can::addAcceptanceFilter;
using openxc::can::read::sendNumericalMessage;
using openxc::pipeline::Pipeline;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;

namespace time = openxc::util::time;

static ActiveRequestListEntry* popListEntry(ActiveRequestList* list) {
    ActiveRequestListEntry* result = list->lh_first;
    if(result != NULL) {
        LIST_REMOVE(list->lh_first, entries);
    }
    return result;
}

static bool sendDiagnosticCanMessage(CanBus* bus,
        const uint16_t arbitrationId, const uint8_t* data,
        const uint8_t size) {
    CanMessage message = {
        id: arbitrationId,
        data: get_bitfield(data, size, 0, size * CHAR_BIT)
            << (64 - CHAR_BIT * size),
        length: size
    };
    openxc::can::write::enqueueMessage(bus, &message);
    return true;
}

static bool sendDiagnosticCanMessageBus1(
        const uint16_t arbitrationId, const uint8_t* data,
        const uint8_t size) {
    return sendDiagnosticCanMessage(&getCanBuses()[0], arbitrationId, data,
            size);
}

static bool sendDiagnosticCanMessageBus2(
        const uint16_t arbitrationId, const uint8_t* data,
        const uint8_t size) {
    return sendDiagnosticCanMessage(&getCanBuses()[1], arbitrationId, data,
            size);
}

void openxc::diagnostics::initialize(DiagnosticsManager* manager, CanBus* buses,
        int busCount) {
    if(busCount > 0) {
        manager->shims[0]= diagnostic_init_shims(openxc::util::log::debug,
                           sendDiagnosticCanMessageBus1, NULL);
    }

    if(busCount > 1) {
        manager->shims[1]= diagnostic_init_shims(openxc::util::log::debug,
                           sendDiagnosticCanMessageBus2, NULL);
    }

    LIST_INIT(&manager->activeRequests);
    LIST_INIT(&manager->freeActiveRequests);

    for(int i = 0; i < MAX_SIMULTANEOUS_DIAG_REQUESTS; i++) {
        LIST_INSERT_HEAD(&manager->freeActiveRequests,
                &manager->activeListEntries[i], entries);
    }
}

static inline bool requestShouldRecur(ActiveDiagnosticRequest* request) {
    // TODO do we want to check if the previous request has been completed? if
    // we wait for that, it's possible we could get stuck - if we made the
    // request while the CAN bus wasn't up, we're not going to get a response.
    // unless the frequency is > 10Hz, there's little chance that we wouldn't
    // have receive the response by the next tick, so if it isn't completed it
    // likely means we're not going to hear back. for now, let's try again.
    return request->recurring && time::shouldTick(&request->frequencyClock);
}

void openxc::diagnostics::sendRequests(DiagnosticsManager* manager,
        CanBus* bus) {
    for(ActiveRequestListEntry* entry = manager->activeRequests.lh_first;
            entry != NULL; entry = entry->entries.le_next) {
        if(entry->request.bus == bus && requestShouldRecur(&entry->request)) {
            entry->request.handle = diagnostic_request(
                    // TODO eek, is bus address and array index this tightly
                    // coupled?
                    &manager->shims[bus->address - 1],
                    &entry->request.handle.request,
                    NULL);
        }
    }
}

static openxc_VehicleMessage wrapDiagnosticResponseWithSabot(CanBus* bus,
        const DiagnosticResponse* response) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_DIAGNOSTIC;
    message.has_diagnostic_message = true;
    message.diagnostic_message = {0};
    message.diagnostic_message.has_bus = true;
    message.diagnostic_message.bus = bus->address;
    message.diagnostic_message.has_message_id = true;
    message.diagnostic_message.message_id = response->arbitration_id;
    message.diagnostic_message.has_mode = true;
    message.diagnostic_message.mode = response->mode;
    message.diagnostic_message.has_pid = response->has_pid;
    if(message.diagnostic_message.has_pid) {
        message.diagnostic_message.pid = response->pid;
    }
    message.diagnostic_message.has_success = true;
    message.diagnostic_message.success = response->success;
    message.diagnostic_message.has_negative_response_code = !response->success;
    message.diagnostic_message.negative_response_code =
            response->negative_response_code;
    message.diagnostic_message.has_payload = response->payload_length > 0;
    memcpy(message.diagnostic_message.payload.bytes, response->payload,
            response->payload_length);
    message.diagnostic_message.payload.size = response->payload_length;

    return message;
}

static void relayDiagnosticResponse(ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response, Pipeline* pipeline) {
    debug("Diagnostic response received: arb_id: 0x%02x, mode: 0x%x, pid: 0x%x, payload: 0x%02x%02x%02x%02x%02x%02x%02x, size: %d",
            response->arbitration_id,
            response->mode,
            response->pid,
            response->payload[0],
            response->payload[1],
            response->payload[2],
            response->payload[3],
            response->payload[4],
            response->payload[5],
            response->payload[6],
            response->payload_length);

    if(strnlen(request->genericName, sizeof(request->genericName)) > 0) {
        float value;
        if(request->decoder != NULL) {
            value = request->decoder(response);
        } else {
            value = diagnostic_payload_to_float(response);
        }
        sendNumericalMessage(request->genericName, value, pipeline);
    } else {
        openxc_VehicleMessage message = wrapDiagnosticResponseWithSabot(
                request->bus, response);
        openxc::can::read::sendVehicleMessage(&message, pipeline);
    }
}

void openxc::diagnostics::receiveCanMessage(DiagnosticsManager* manager,
        CanBus* bus, CanMessage* message, Pipeline* pipeline) {
    // TODO instead of checking here if a handle is completed, we always pass
    // messages on to everything in the list. on cleanup, if a handle is
    // completed (ie. a response has been received) or it's braodcast type and
    // it's been more than 100ms, remove it.
    //
    // problem: we make one request to a node for mode 1. we get the response,
    // it's marked completed.
    // we make a mode 2 request to the same node before removing it from the
    // list. we get the response - the old request has it and see's its for the
    // wrong mode, it re-requests.
    //
    // solution: don't re-request, you are finished.
    for(ActiveRequestListEntry* entry = manager->activeRequests.lh_first;
            entry != NULL; entry = entry->entries.le_next) {

        if(bus != entry->request.bus) {
            continue;
        }

        ArrayOrBytes combined;
        combined.whole = message->data;
        DiagnosticResponse response = diagnostic_receive_can_frame(
                &manager->shims[bus->address - 1],
                &entry->request.handle, message->id, combined.bytes,
                sizeof(combined.bytes));
        if(response.completed && entry->request.handle.completed) {
            if(entry->request.handle.success) {
                if(response.success) {
                    relayDiagnosticResponse(&entry->request, &response, pipeline);
                    // TODO remove a CAN filter if nothing else is waiting
                    // on it...hard to tell.
                } else {
                    debug("Negative diagnostic response received, NRC: 0x%x",
                            response.negative_response_code);
                }
            } else {
                debug("Fatal error when sending or receiving diagnostic request");
            }
        }
    }
}

static void cleanupActiveRequests(DiagnosticsManager* manager) {
    // clean up the active request list, move as many to the free list as
    // possible
    for(ActiveRequestListEntry* entry = manager->activeRequests.lh_first;
            entry != NULL; entry = entry->entries.le_next) {
        ActiveDiagnosticRequest* request = &entry->request;
        if(request->recurring) {
            // TODO leave active unless explicitly removed
        } else if(request->arbitration_id == OBD2_FUNCTIONAL_BROADCAST_ID) {
            // TODO when request is > 100ms old, remove it...or we've received a
            // few responses? nah, timeout.
        } else if(request->handle.completed) {
            LIST_REMOVE(entry, entries);
            LIST_INSERT_HEAD(&manager->freeActiveRequests, entry, entries);
        }
    }
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* genericName,
        const DiagnosticResponseDecoder decoder, const uint8_t frequencyHz) {

    if(genericName == NULL && decoder != NULL) {
        return false;
    }

    ActiveRequestListEntry* newEntry = popListEntry(
            &manager->freeActiveRequests);
    if(newEntry == NULL) {
        cleanupActiveRequests(manager);
        newEntry = popListEntry(&manager->freeActiveRequests);
    }

    if(newEntry == NULL) {
        debug("Unable to allocate space for a new diagnostic request");
        // TODO at the moment we're *never* deleting requests that were going to
        // the broadcast address, because they're never "completed" unless we
        // time them out while waiting for response from multiple modules
        return false;
    }

    bool filterStatus = true;
    if(request->arbitration_id == OBD2_FUNCTIONAL_BROADCAST_ID) {
        for(uint16_t filter = OBD2_FUNCTIONAL_RESPONSE_START; filter <
                OBD2_FUNCTIONAL_RESPONSE_START + OBD2_FUNCTIONAL_RESPONSE_COUNT;
                filter++) {
            filterStatus = filterStatus && addAcceptanceFilter(bus, filter,
                    getCanBuses(), getCanBusCount());
        }
    } else {
        filterStatus = addAcceptanceFilter(bus, request->arbitration_id, getCanBuses(), getCanBusCount());
    }

    if(!filterStatus) {
        debug("Couldn't add filter 0x%x to bus %d", request->arbitration_id, bus->address);
        LIST_INSERT_HEAD(&manager->freeActiveRequests, newEntry, entries);
        return false;
    }

    newEntry->request.bus = bus;
    newEntry->request.handle = diagnostic_request(
            &manager->shims[bus->address - 1], request, NULL);
    if(genericName != NULL) {
        strncpy(newEntry->request.genericName, genericName, MAX_GENERIC_NAME_LENGTH);
    } else {
        newEntry->request.genericName[0] = '\0';
    }
    newEntry->request.decoder = decoder;
    newEntry->request.recurring = frequencyHz != 0;
    // TODO we could (ab)use the frequency clock for non-recurring requests and
    // use it as a timeout - if we set the frequency to 1Hz, when it "should
    // tick" and it's not yet completed, we know a response hasn't been received
    // in 1 second and we should either kill it or retry
    newEntry->request.frequencyClock = {0};
    newEntry->request.frequencyClock.frequency =
            newEntry->request.recurring ? frequencyHz : 1;
    LIST_INSERT_HEAD(&manager->activeRequests, newEntry, entries);

    return true;
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* genericName,
        const DiagnosticResponseDecoder decoder) {
    return addDiagnosticRequest(manager, bus, request, genericName, decoder, 0);
}
