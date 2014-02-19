#include "diagnostics.h"
#include "signals.h"
#include "can/canwrite.h"
#include "can/canread.h"
#include "util/log.h"
#include "util/timer.h"
#include <bitfield/bitfield.h>
#include <limits.h>

#define MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ 10
#define DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET 0x8

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
        if(busCount > 1) {
            manager->shims[1]= diagnostic_init_shims(openxc::util::log::debug,
                    sendDiagnosticCanMessageBus2, NULL);
        }
    }


    LIST_INIT(&manager->activeRequests);
    LIST_INIT(&manager->freeActiveRequests);

    for(int i = 0; i < MAX_SIMULTANEOUS_DIAG_REQUESTS; i++) {
        LIST_INSERT_HEAD(&manager->freeActiveRequests,
                &manager->activeListEntries[i], entries);
    }
}

static inline bool requestShouldRecur(ActiveDiagnosticRequest* request) {
    // Not currently checking if the request is completed or not. If
    // we wait for that, it's possible we could get stuck - if we made the
    // request while the CAN bus wasn't up, we're not going to get a response.
    // unless the frequency is > 10Hz, there's little chance that we wouldn't
    // have receive the response by the next tick, so if it isn't completed it
    // likely means we're not going to hear back. We'll send the request again.
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
            entry->request.timeoutClock = {0};
        }
    }
}

static openxc_VehicleMessage wrapDiagnosticResponseWithSabot(CanBus* bus,
        const ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response, float parsedValue) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_DIAGNOSTIC;
    message.has_diagnostic_message = true;
    message.diagnostic_message = {0};
    message.diagnostic_message.has_bus = true;
    message.diagnostic_message.bus = bus->address;
    message.diagnostic_message.has_message_id = true;

    if(request->arbitration_id != OBD2_FUNCTIONAL_BROADCAST_ID) {
        message.diagnostic_message.message_id = response->arbitration_id
            - DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET;
    } else {
        // must preserve responding arb ID for responses to functional broadcast
        // requests, as they are the actual module address and not just arb ID +
        // 8.
        message.diagnostic_message.message_id = response->arbitration_id;
    }

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

    if(message.diagnostic_message.has_payload && request->parsePayload) {
        message.diagnostic_message.has_value = true;
        message.diagnostic_message.value = parsedValue;
    }

    return message;
}

static void relayDiagnosticResponse(ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response, Pipeline* pipeline) {
    float value = diagnostic_payload_to_integer(response) *
            request->factor + request->offset;
    if(request->decoder != NULL) {
        value = request->decoder(response, value);
    }
    if(response->success && strnlen(
                request->genericName, sizeof(request->genericName)) > 0) {
        sendNumericalMessage(request->genericName, value, pipeline);
    } else {
        openxc_VehicleMessage message = wrapDiagnosticResponseWithSabot(
                request->bus, request, response, value);
        openxc::can::read::sendVehicleMessage(&message, pipeline);
    }
}

void openxc::diagnostics::receiveCanMessage(DiagnosticsManager* manager,
        CanBus* bus, CanMessage* message, Pipeline* pipeline) {
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
                relayDiagnosticResponse(&entry->request, &response, pipeline);
            } else {
                debug("Fatal error when sending or receiving diagnostic request");
            }
        }
    }
}

static bool broadcastTimedOut(ActiveDiagnosticRequest* request) {
    return request->arbitration_id == OBD2_FUNCTIONAL_BROADCAST_ID
            && time::shouldTick(&request->timeoutClock);
}

static void cleanupActiveRequests(DiagnosticsManager* manager) {
    // clean up the active request list, move as many to the free list as
    // possible
    for(ActiveRequestListEntry* entry = manager->activeRequests.lh_first;
            entry != NULL; entry = entry->entries.le_next) {
        ActiveDiagnosticRequest* request = &entry->request;
        if(!request->recurring && (
                broadcastTimedOut(request) ||
                (request->arbitration_id == OBD2_FUNCTIONAL_BROADCAST_ID
                    && request->handle.completed))) {
            LIST_REMOVE(entry, entries);
            LIST_INSERT_HEAD(&manager->freeActiveRequests, entry, entries);
        }
    }
}

static bool addDiagnosticRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* genericName,
        bool parsePayload, float factor, float offset,
        const openxc::diagnostics::DiagnosticResponseDecoder decoder,
        const uint8_t frequencyHz) {

    if(frequencyHz > MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ) {
        debug("Requested recurring diagnostic frequency %d is higher than maximum of %d",
                frequencyHz, MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ);
        return false;
    }

    cleanupActiveRequests(manager);
    ActiveRequestListEntry* newEntry = popListEntry(
            &manager->freeActiveRequests);
    if(newEntry == NULL) {
        debug("Unable to allocate space for a new diagnostic request");
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
        filterStatus = addAcceptanceFilter(bus,
                request->arbitration_id +
                        DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET,
                getCanBuses(), getCanBusCount());
    }

    if(!filterStatus) {
        debug("Couldn't add filter 0x%x to bus %d", request->arbitration_id, bus->address);
        LIST_INSERT_HEAD(&manager->freeActiveRequests, newEntry, entries);
        return false;
    }

    newEntry->request.bus = bus;
    newEntry->request.arbitration_id = request->arbitration_id;
    newEntry->request.handle = diagnostic_request(
            &manager->shims[bus->address - 1], request, NULL);
    if(genericName != NULL) {
        strncpy(newEntry->request.genericName, genericName, MAX_GENERIC_NAME_LENGTH);
    } else {
        newEntry->request.genericName[0] = '\0';
    }
    newEntry->request.parsePayload = parsePayload;
    newEntry->request.factor = factor;
    newEntry->request.offset = offset;
    newEntry->request.decoder = decoder;
    newEntry->request.recurring = frequencyHz != 0;
    newEntry->request.frequencyClock = {0};
    newEntry->request.frequencyClock.frequency =
            newEntry->request.recurring ? frequencyHz : 1;
    // time out after 100ms
    newEntry->request.timeoutClock = {0};
    newEntry->request.timeoutClock.frequency = 10;
    LIST_INSERT_HEAD(&manager->activeRequests, newEntry, entries);

    return true;
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request, const char* genericName,
        float factor, float offset, const DiagnosticResponseDecoder decoder,
        const uint8_t frequencyHz) {
    return addDiagnosticRequest(manager, bus, request, genericName, true, factor,
            offset, decoder, frequencyHz);
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request, const uint8_t frequencyHz) {
    return addDiagnosticRequest(manager, bus, request, NULL, false, 1.0, 0,
            NULL, frequencyHz);
}
