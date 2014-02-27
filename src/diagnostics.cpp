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

using openxc::diagnostics::ActiveDiagnosticRequest;
using openxc::diagnostics::DiagnosticRequestListEntry;
using openxc::diagnostics::DiagnosticsManager;
using openxc::util::log::debug;
using openxc::can::lookupBus;
using openxc::can::addAcceptanceFilter;
using openxc::can::removeAcceptanceFilter;
using openxc::can::read::sendNumericalMessage;
using openxc::pipeline::Pipeline;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;

namespace time = openxc::util::time;

static bool timedOut(ActiveDiagnosticRequest* request) {
    return time::elapsed(&request->timeoutClock, true);
}

/* Private: Returns true if the request has timed out waiting for a response,
 *      or the request handle is completed and it wasn't a functional broadcast
 *      request. Functional broadcast requests are never complete until they
 *      time out.
 */
static bool requestCompleted(ActiveDiagnosticRequest* request) {
    return (request->arbitration_id != OBD2_FUNCTIONAL_BROADCAST_ID &&
                 request->handle.completed) || timedOut(request);
}

/* Private: Move the entry to the free list and decrement the lock count for any
 * CAN filters it used.
 */
static void cancelRequest(DiagnosticsManager* manager,
        DiagnosticRequestListEntry* entry) {
    LIST_INSERT_HEAD(&manager->freeActiveRequests, entry, listEntries);
    removeAcceptanceFilter(entry->request.bus,
            entry->request.arbitration_id + DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET,
            getCanBuses(), getCanBusCount());
}

// clean up the active request list, move as many to the free list as
// possible
static void cleanupActiveRequests(DiagnosticsManager* manager) {
    DiagnosticRequestListEntry* entry, *tmp;
    LIST_FOREACH_SAFE(entry, &manager->inFlightRequests, listEntries, tmp) {
        ActiveDiagnosticRequest* request = &entry->request;
        if(requestCompleted(&entry->request)) {
            char request_string[128] = {0};
            diagnostic_request_to_string(&request->handle.request,
                    request_string, sizeof(request_string));
            debug("Moving timed out or completed request back to active list: %s", request_string);

            LIST_REMOVE(entry, listEntries);
            TAILQ_INSERT_TAIL(&manager->activeRequests, entry, queueEntries);
        }
    }

    TAILQ_FOREACH_SAFE(entry, &manager->activeRequests, queueEntries, tmp) {
        ActiveDiagnosticRequest* request = &entry->request;
        if(!request->recurring && requestCompleted(request)) {
            char request_string[128] = {0};
            diagnostic_request_to_string(&request->handle.request,
                    request_string, sizeof(request_string));
            debug("Cancelling timed out or completed, non-recurring request: %s", request_string);
            TAILQ_REMOVE(&manager->activeRequests, entry, queueEntries);
            cancelRequest(manager, entry);
        }
    }
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

    TAILQ_INIT(&manager->activeRequests);
    LIST_INIT(&manager->inFlightRequests);
    LIST_INIT(&manager->freeActiveRequests);

    for(int i = 0; i < MAX_SIMULTANEOUS_DIAG_REQUESTS; i++) {
        LIST_INSERT_HEAD(&manager->freeActiveRequests,
                &manager->requestListEntries[i], listEntries);
    }
}

/* Private: Returns true if there are no other active requests to the same arb
 * ID.
 */
static inline bool clearToSend(DiagnosticsManager* manager,
        ActiveDiagnosticRequest* request) {
    DiagnosticRequestListEntry* entry;
    LIST_FOREACH(entry, &manager->inFlightRequests, listEntries) {
        if(&entry->request != request &&
                entry->request.arbitration_id == request->arbitration_id) {
            return false;
        }
    }
    return true;
}

static inline bool shouldSend(ActiveDiagnosticRequest* request) {
    return (!request->recurring && !requestCompleted(request)) ||
            (request->recurring &&
                time::elapsed(&request->frequencyClock, true));
}

void openxc::diagnostics::sendRequests(DiagnosticsManager* manager,
        CanBus* bus) {
    DiagnosticRequestListEntry* entry, *tmp;
    TAILQ_FOREACH_SAFE(entry, &manager->activeRequests, queueEntries, tmp) {
        ActiveDiagnosticRequest* request = &entry->request;
        if(request->bus == bus && shouldSend(request) &&
                    clearToSend(manager, request)) {
            time::tick(&request->frequencyClock);
            start_diagnostic_request(&manager->shims[bus->address - 1],
                    &request->handle);
            request->timeoutClock = {0};
            request->timeoutClock.frequency = 10;

            TAILQ_REMOVE(&manager->activeRequests, entry, queueEntries);
            LIST_INSERT_HEAD(&manager->inFlightRequests, entry, listEntries);
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

void openxc::diagnostics::loop(DiagnosticsManager* manager) {
    cleanupActiveRequests(manager);
}

void openxc::diagnostics::receiveCanMessage(DiagnosticsManager* manager,
        CanBus* bus, CanMessage* message, Pipeline* pipeline) {
    DiagnosticRequestListEntry* entry, *tmp;
    LIST_FOREACH_SAFE(entry, &manager->inFlightRequests, listEntries, tmp) {
        if(bus != entry->request.bus) {
            continue;
        }

        ArrayOrBytes combined;
        combined.whole = message->data;
        DiagnosticResponse response = diagnostic_receive_can_frame(
                // TODO eek, is bus address and array index this tightly
                // coupled?
                &manager->shims[bus->address - 1],
                &entry->request.handle, message->id, combined.bytes,
                sizeof(combined.bytes));
        if(response.completed && entry->request.handle.completed) {
            if(entry->request.handle.success) {
                relayDiagnosticResponse(&entry->request, &response, pipeline);

                LIST_REMOVE(entry, listEntries);
                TAILQ_INSERT_TAIL(&manager->activeRequests, entry,
                        queueEntries);
            } else {
                debug("Fatal error when sending or receiving diagnostic request");
            }
        }
    }
    cleanupActiveRequests(manager);
}

static bool addDiagnosticRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* genericName,
        bool parsePayload, float factor, float offset,
        const openxc::diagnostics::DiagnosticResponseDecoder decoder,
        float frequencyHz) {

    if(frequencyHz > MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ) {
        debug("Requested recurring diagnostic frequency %d is higher than maximum of %d",
                frequencyHz, MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ);
        return false;
    }

    cleanupActiveRequests(manager);
    DiagnosticRequestListEntry* newEntry = LIST_FIRST(&manager->freeActiveRequests);
    if(newEntry == NULL) {
        debug("Unable to allocate space for a new diagnostic request");
        return false;
    }
    LIST_REMOVE(newEntry, listEntries);

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
        LIST_INSERT_HEAD(&manager->freeActiveRequests, newEntry, listEntries);
        return false;
    }

    newEntry->request.bus = bus;
    newEntry->request.arbitration_id = request->arbitration_id;
    newEntry->request.handle = generate_diagnostic_request(
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
    TAILQ_INSERT_HEAD(&manager->activeRequests, newEntry, queueEntries);

    return true;
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request, const char* genericName,
        float factor, float offset, const DiagnosticResponseDecoder decoder,
        float frequencyHz) {
    return addDiagnosticRequest(manager, bus, request, genericName, true, factor,
            offset, decoder, frequencyHz);
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request, float frequencyHz) {
    return addDiagnosticRequest(manager, bus, request, NULL, false, 1.0, 0,
            NULL, frequencyHz);
}

void openxc::diagnostics::handleDiagnosticCommand(
        DiagnosticsManager* diagnosticsManager, uint8_t* payload) {
    cJSON *root = cJSON_Parse((char*)payload);
    if(root != NULL) {
        cJSON* commandNameObject = cJSON_GetObjectItem(root, "command");
        if(commandNameObject != NULL &&
                !strcmp(commandNameObject->valuestring, "diagnostic_request")) {
            cJSON* requestObject = cJSON_GetObjectItem(root, "request");
            if(requestObject != NULL) {
                cJSON* busObject = cJSON_GetObjectItem(requestObject, "bus");
                cJSON* idObject = cJSON_GetObjectItem(requestObject, "id");
                cJSON* modeObject = cJSON_GetObjectItem(requestObject, "mode");
                cJSON* pidObject = cJSON_GetObjectItem(requestObject, "pid");
                cJSON* frequencyObject = cJSON_GetObjectItem(requestObject, "frequency");

                if(busObject != NULL && idObject != NULL && modeObject != NULL) {
                    CanBus* canBus = lookupBus(busObject->valueint,
                            getCanBuses(), getCanBusCount());
                    if(canBus == NULL) {
                        debug("No matching active bus for requested address: %d", busObject->valueint);
                        return;
                    }

                    DiagnosticRequest request = {
                        arbitration_id: uint16_t(idObject->valueint),
                        mode: uint8_t(modeObject->valueint),
                        has_pid: true,
                        pid: uint16_t(pidObject->valueint),
                        pid_length: 0};
                        // TODO other fields

                    float frequency = 0;
                    if(frequencyObject != NULL) {
                        frequency = frequencyObject->valuedouble;
                    }

                    // TODO grab name, min max, use other constructor if needed
                    addDiagnosticRequest(diagnosticsManager, canBus, &request,
                            frequency);
                }
            }
        }
        cJSON_Delete(root);
    }
}
