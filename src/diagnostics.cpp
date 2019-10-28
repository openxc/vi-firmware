#include "diagnostics.h"
#include "signals.h"
#include "can/canwrite.h"
#include "can/canread.h"
#include "util/log.h"
#include "util/timer.h"
#include "obd2.h"
#include <bitfield/bitfield.h>
#include <limits.h>
#include "config.h"

#define MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ 10
#define DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET 0x8

using openxc::diagnostics::ActiveDiagnosticRequest;
using openxc::diagnostics::DiagnosticsManager;
using openxc::diagnostics::DiagnosticResponseDecoder;
using openxc::diagnostics::DiagnosticResponseCallback;
using openxc::diagnostics::passthroughDecoder;
using openxc::util::log::debug;
using openxc::can::lookupBus;
using openxc::can::addAcceptanceFilter;
using openxc::can::removeAcceptanceFilter;
using openxc::can::read::publishNumericalMessage;
using openxc::can::read::publishStringMessage;
using openxc::pipeline::Pipeline;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::config::getConfiguration;

namespace time = openxc::util::time;
namespace pipeline = openxc::pipeline;
namespace obd2 = openxc::diagnostics::obd2;

static bool timedOut(ActiveDiagnosticRequest* request) {
    // don't use staggered start with the timeout clock
    return time::elapsed(&request->timeoutClock, false);
}

/* Private: Returns true if a sufficient response has been received for a
 * diagnostic request.
 *
 * This is true when at least one response has been received and the request is
 * configured to not wait for multiple responses. Functional broadcast requests
 * may often wish to wait the full 100ms for modules to respond.
 */
static bool responseReceived(ActiveDiagnosticRequest* request) {
    return !request->waitForMultipleResponses &&
                request->handle.completed;
}

/* Private: Returns true if the request has timed out waiting for a response,
 *      or a sufficient number of responses has been received.
 */
static bool requestCompleted(ActiveDiagnosticRequest* request) {
    return responseReceived(request) || (
            timedOut(request) && diagnostic_request_sent(&request->handle));
}

/* Private: Move the entry to the free list and decrement the lock count for any
 * CAN filters it used.
 */
static void cancelRequest(DiagnosticsManager* manager,
        ActiveDiagnosticRequest* entry) {
    LIST_INSERT_HEAD(&manager->freeRequestEntries, entry, listEntries);
    if(entry->arbitration_id == OBD2_FUNCTIONAL_BROADCAST_ID) {
        for(uint32_t filter = OBD2_FUNCTIONAL_RESPONSE_START;
                filter < OBD2_FUNCTIONAL_RESPONSE_START +
                    OBD2_FUNCTIONAL_RESPONSE_COUNT;
                filter++) {
            removeAcceptanceFilter(entry->bus, filter,
                    CanMessageFormat::STANDARD, getCanBuses(),
                    getCanBusCount());
        }
    } else {
        removeAcceptanceFilter(entry->bus,
                entry->arbitration_id +
                    DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET,
                CanMessageFormat::STANDARD, getCanBuses(), getCanBusCount());
    }
}

static void cleanupRequest(DiagnosticsManager* manager,
        ActiveDiagnosticRequest* entry, bool force) {
    if(force || (entry->inFlight && requestCompleted(entry))) {
        entry->inFlight = false;

        char request_string[128] = {0};
        diagnostic_request_to_string(&entry->handle.request,
                request_string, sizeof(request_string));
        if(entry->recurring) {
            TAILQ_REMOVE(&manager->recurringRequests, entry, queueEntries);
            if(force) {
                cancelRequest(manager, entry);
            } else {
                debug("Moving completed recurring request to the back "
                        "of the queue: %s", request_string);
                TAILQ_INSERT_TAIL(&manager->recurringRequests, entry,
                        queueEntries);
            }
        } else {
            debug("Cancelling completed, non-recurring request: %s",
                    request_string);
            LIST_REMOVE(entry, listEntries);
            cancelRequest(manager, entry);
        }
    }
}

// clean up the request list, move as many to the free list as possible
static void cleanupActiveRequests(DiagnosticsManager* manager, bool force) {
    ActiveDiagnosticRequest* entry, *tmp;
    LIST_FOREACH_SAFE(entry, &manager->nonrecurringRequests, listEntries, tmp) {
        cleanupRequest(manager, entry, force);
    }

    TAILQ_FOREACH_SAFE(entry, &manager->recurringRequests, queueEntries, tmp) {
        cleanupRequest(manager, entry, force);
    }
}

static bool sendDiagnosticCanMessage(CanBus* bus,
        const uint32_t arbitrationId, const uint8_t data[],
        const uint8_t size) {
    CanMessage message = {
        id: arbitrationId,
        format: arbitrationId > 2047 ?
            CanMessageFormat::EXTENDED : CanMessageFormat::STANDARD,
        data: {0},
        length: size
    };
    memcpy(message.data, data, size);
    openxc::can::write::enqueueMessage(bus, &message);
    return true;
}

static bool sendDiagnosticCanMessageBus1(
        const uint32_t arbitrationId, const uint8_t* data,
        const uint8_t size) {
    return sendDiagnosticCanMessage(&getCanBuses()[0], arbitrationId, data,
            size);
}

static bool sendDiagnosticCanMessageBus2(
        const uint32_t arbitrationId, const uint8_t* data,
        const uint8_t size) {
    return sendDiagnosticCanMessage(&getCanBuses()[1], arbitrationId, data,
            size);
}

void openxc::diagnostics::reset(DiagnosticsManager* manager) {
    if(manager->initialized) {
        debug("Clearing existing diagnostic requests");
        cleanupActiveRequests(manager, true);
    }

    TAILQ_INIT(&manager->recurringRequests);
    LIST_INIT(&manager->nonrecurringRequests);
    LIST_INIT(&manager->freeRequestEntries);

    for(int i = 0; i < MAX_SIMULTANEOUS_DIAG_REQUESTS; i++) {
        LIST_INSERT_HEAD(&manager->freeRequestEntries,
                &manager->requestListEntries[i], listEntries);
    }

    debug("Reset diagnostics requests");
}

void openxc::diagnostics::initialize(DiagnosticsManager* manager, CanBus* buses,
        int busCount, uint8_t obd2BusAddress) {
    if(busCount > 0) {
        manager->shims[0] = diagnostic_init_shims(openxc::util::log::debug,
                sendDiagnosticCanMessageBus1, NULL);
        if(busCount > 1) {
            manager->shims[1] = diagnostic_init_shims(openxc::util::log::debug,
                    sendDiagnosticCanMessageBus2, NULL);
        }
    }

    reset(manager);
    manager->initialized = true;

    manager->obd2Bus = lookupBus(obd2BusAddress, buses, busCount);
    obd2::initialize(manager);
    debug("Initialized diagnostics");
}

static inline bool conflicting(ActiveDiagnosticRequest* request,
        ActiveDiagnosticRequest* candidate) {
    return (candidate->inFlight && candidate != request &&
            candidate->bus == request->bus &&
            candidate->arbitration_id == request->arbitration_id);
}


/* Private: Returns true if there are no other active requests to the same arb
 * ID.
 */
static inline bool clearToSend(DiagnosticsManager* manager,
        ActiveDiagnosticRequest* request) {
    ActiveDiagnosticRequest* entry;
    LIST_FOREACH(entry, &manager->nonrecurringRequests, listEntries) {
        if(conflicting(request, entry)) {
            return false;
        }
    }

    TAILQ_FOREACH(entry, &manager->recurringRequests, queueEntries) {
        if(conflicting(request, entry)) {
            return false;
        }
    }

    return true;
}

static inline bool shouldSend(ActiveDiagnosticRequest* request) {
    return !request->inFlight && (
            (!request->recurring && !requestCompleted(request)) ||
            (request->recurring && time::elapsed(&request->frequencyClock,
                                                 true)));
}

static void sendRequest(DiagnosticsManager* manager, CanBus* bus,
        ActiveDiagnosticRequest* request) {
    if(request->bus == bus && shouldSend(request) &&
            clearToSend(manager, request)) {
        time::tick(&request->frequencyClock);
        start_diagnostic_request(&manager->shims[bus->address - 1],
                &request->handle);
        if(request->handle.completed && !request->handle.success) {
            debug("Fatal error sending diagnostic request");
        } else {
            request->timeoutClock = {0};
            request->timeoutClock.frequency = 10;
            time::tick(&request->timeoutClock);
            request->inFlight = true;
        }
    }
}

void openxc::diagnostics::sendRequests(DiagnosticsManager* manager,
        CanBus* bus) {
    cleanupActiveRequests(manager, false);

    ActiveDiagnosticRequest* entry;
    LIST_FOREACH(entry, &manager->nonrecurringRequests, listEntries) {
        sendRequest(manager, bus, entry);
    }

    TAILQ_FOREACH(entry, &manager->recurringRequests, queueEntries) {
        sendRequest(manager, bus, entry);
    }
}

static openxc_VehicleMessage wrapDiagnosticResponseWithSabot(CanBus* bus,
        const ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response, openxc_DynamicField value) {
    openxc_VehicleMessage message = openxc_VehicleMessage();		// Zero fill
    message.type = openxc_VehicleMessage_Type_DIAGNOSTIC;
    message.diagnostic_response = {0};
    message.diagnostic_response.bus = bus->address;

    if(request->arbitration_id != OBD2_FUNCTIONAL_BROADCAST_ID) {
        message.diagnostic_response.message_id = response->arbitration_id
            - DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET;
    } else {
        // must preserve responding arb ID for responses to functional broadcast
        // requests, as they are the actual module address and not just arb ID +
        // 8.
        message.diagnostic_response.message_id = response->arbitration_id;
    }

    message.diagnostic_response.mode = response->mode;
    if(message.diagnostic_response.pid != 0) {
        message.diagnostic_response.pid = response->pid;
    }
    message.diagnostic_response.success = response->success;
    message.diagnostic_response.negative_response_code =
            response->negative_response_code;

    if(response->payload_length > 0) {
        if (request->decoder != NULL)  {
            message.diagnostic_response.value = value;
        } else {
            memcpy(message.diagnostic_response.payload.bytes, response->payload,
                    response->payload_length);
            message.diagnostic_response.payload.size = response->payload_length;
        }
    }

    return message;
}

static void relayDiagnosticResponse(DiagnosticsManager* manager,
        ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response, Pipeline* pipeline) {
    float parsed_value = diagnostic_payload_to_integer(response);

    uint8_t buf_size = response->multi_frame ? response->payload_length + 1 : 20;
    char decoded_value_buf[buf_size];

    bool has_decoder = NULL != request->decoder;
    if (has_decoder) {
        request->decoder(response, parsed_value, decoded_value_buf, buf_size);
    }

    openxc_DynamicField field = openxc_DynamicField();    // Zero fill
    if (response->multi_frame) {
        field.type = openxc_DynamicField_Type_STRING;
        if (!has_decoder) {
            snprintf(decoded_value_buf, buf_size, "%s", response->payload);
        }
        strcpy(field.string_value, decoded_value_buf);
    } else {
        field.type = openxc_DynamicField_Type_NUM;
        if (!has_decoder) {
            snprintf(decoded_value_buf, buf_size, "%f", parsed_value);
        }
        field.numeric_value = atof(decoded_value_buf);
    }

    if(response->success && strnlen(request->name, sizeof(request->name)) > 0) {
        // If name, include 'value' instead of payload, and leave of response
        // details.
        if (strlen(field.string_value) > 0) {
            publishStringMessage(request->name, field.string_value, pipeline);
        } else {
            publishNumericalMessage(request->name, field.numeric_value, pipeline);
        }
    } else {
        // If no name, send full details of response but still include 'value'
        // instead of 'payload' if they provided a decoder. The one case you
        // can't get is the full detailed response with 'value'. We could add
        // another parameter for that but it's onerous to carry that around.
        openxc_VehicleMessage message = wrapDiagnosticResponseWithSabot(
                request->bus, request, response, field);
        pipeline::publish(&message, pipeline);
    }

    if(request->callback != NULL) {
        request->callback(manager, request, response, parsed_value);
    }
}

static void receiveCanMessage(DiagnosticsManager* manager,
        CanBus* bus,
        ActiveDiagnosticRequest* entry,
        CanMessage* message, Pipeline* pipeline) {
    if (bus == entry->bus && entry->inFlight) {
        DiagnosticResponse response = diagnostic_receive_can_frame(
                // TODO eek, is bus address and array index this tightly
                // coupled?
                &manager->shims[bus->address - 1],
                &entry->handle, message->id, message->data, message->length);
        if (response.completed && entry->handle.completed) {
            if(entry->handle.success) {
                relayDiagnosticResponse(manager, entry, &response,
                        pipeline);
            } else {
                debug("Fatal error sending or receiving diagnostic request");
            }
        } else if (!response.completed && response.multi_frame) {
            // Reset the timeout clock while completing the multi-frame receive
            time::tick(&entry->timeoutClock);
        }
    }
}

void openxc::diagnostics::receiveCanMessage(DiagnosticsManager* manager,
        CanBus* bus, CanMessage* message, Pipeline* pipeline) {
    ActiveDiagnosticRequest* entry;

    TAILQ_FOREACH(entry, &manager->recurringRequests, queueEntries) {
        receiveCanMessage(manager, bus, entry, message, pipeline);
    }

    LIST_FOREACH(entry, &manager->nonrecurringRequests, listEntries) {
        receiveCanMessage(manager, bus, entry, message, pipeline);
    }
    cleanupActiveRequests(manager, false);
}

/* Note that this pops it off of whichver list it was on and returns it, so make
 * sure to add it to some other list or it'll be lost.
 */
static ActiveDiagnosticRequest* lookupRecurringRequest(
        DiagnosticsManager* manager, const CanBus* bus,
        const DiagnosticRequest* request) {
    ActiveDiagnosticRequest* existingEntry = NULL, *entry, *tmp;
    TAILQ_FOREACH_SAFE(entry, &manager->recurringRequests, queueEntries, tmp) {
        ActiveDiagnosticRequest* candidate = entry;
        if(candidate->bus == bus && diagnostic_request_equals(
                    &candidate->handle.request, request)) {
            TAILQ_REMOVE(&manager->recurringRequests, entry, queueEntries);
            existingEntry = entry;
            break;
        }
    }
    return existingEntry;
}

bool openxc::diagnostics::cancelRecurringRequest(
        DiagnosticsManager* manager, CanBus* bus, DiagnosticRequest* request) {
    ActiveDiagnosticRequest* entry = lookupRecurringRequest(manager, bus,
            request);
    if(entry != NULL) {
        cancelRequest(manager, entry);
    }
    return entry != NULL;
}

static ActiveDiagnosticRequest* getFreeEntry(DiagnosticsManager* manager) {
    ActiveDiagnosticRequest* entry = LIST_FIRST(&manager->freeRequestEntries);
    // Don't remove it from the free list yet, because there's still an
    // opportunity to fail before we add it to another other list.
    if(entry == NULL) {
        debug("Unable to allocate space for a new diagnostic request");
    }
    return entry;
}

static bool updateRequiredAcceptanceFilters(CanBus* bus,
        DiagnosticRequest* request) {
    bool filterStatus = true;
    if(request->arbitration_id == OBD2_FUNCTIONAL_BROADCAST_ID) {
        for(uint32_t filter = OBD2_FUNCTIONAL_RESPONSE_START;
                filter < OBD2_FUNCTIONAL_RESPONSE_START +
                OBD2_FUNCTIONAL_RESPONSE_COUNT;
                filter++) {
            filterStatus = filterStatus && addAcceptanceFilter(bus, filter,
                    CanMessageFormat::STANDARD, getCanBuses(),
                    getCanBusCount());
        }
    } else {
        filterStatus = addAcceptanceFilter(bus,
                request->arbitration_id +
                DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET,
                CanMessageFormat::STANDARD,
                getCanBuses(), getCanBusCount());
    }

    if(!filterStatus) {
        debug("Couldn't add filter 0x%x to bus %d", request->arbitration_id,
                bus->address);
    }
    return filterStatus;
}

static void updateDiagnosticRequestEntry(ActiveDiagnosticRequest* entry,
        DiagnosticsManager* manager, CanBus* bus, DiagnosticRequest* request,
        const char* name, bool waitForMultipleResponses,
        const DiagnosticResponseDecoder decoder,
        const DiagnosticResponseCallback callback, float frequencyHz) {
    entry->bus = bus;
    entry->arbitration_id = request->arbitration_id;
    entry->handle = generate_diagnostic_request(
            &manager->shims[bus->address - 1], request, NULL);
    if(name != NULL) {
        strncpy(entry->name, name, MAX_GENERIC_NAME_LENGTH);
    } else {
        entry->name[0] = '\0';
    }
    entry->waitForMultipleResponses = waitForMultipleResponses;

    entry->decoder = decoder;
    entry->callback = callback;
    entry->recurring = frequencyHz != 0;
    entry->frequencyClock = {0};
    entry->frequencyClock.frequency = entry->recurring ? frequencyHz : 0;
    // time out after 100ms
    entry->timeoutClock = {0};
    entry->timeoutClock.frequency = 10;
    entry->inFlight = false;
}

bool openxc::diagnostics::addRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, const DiagnosticResponseDecoder decoder,
        const DiagnosticResponseCallback callback) {
    cleanupActiveRequests(manager, false);

    bool added = true;
    ActiveDiagnosticRequest* entry = getFreeEntry(manager);
    if(entry != NULL) {
        if(updateRequiredAcceptanceFilters(bus, request)) {
            updateDiagnosticRequestEntry(entry, manager, bus, request, name,
                    waitForMultipleResponses, decoder, callback, 0);

            char request_string[128] = {0};
            diagnostic_request_to_string(&entry->handle.request, request_string,
                    sizeof(request_string));

            LIST_REMOVE(entry, listEntries);
            debug("Added one-time diagnostic request on bus %d: %s",
                    bus->address, request_string);

            LIST_INSERT_HEAD(&manager->nonrecurringRequests, entry, listEntries);
        } else {
            added = false;
        }
    } else {
        added = false;
    }
    return added;
}

static bool validateOptionalRequestAttributes(float frequencyHz) {
    if(frequencyHz > MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ) {
        debug("Requested recurring diagnostic frequency %d is higher "
                "than maximum of %d", frequencyHz,
                MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ);
        return false;
    }
    return true;
}

bool openxc::diagnostics::addRecurringRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, const DiagnosticResponseDecoder decoder,
        const DiagnosticResponseCallback callback, float frequencyHz) {

    if(!validateOptionalRequestAttributes(frequencyHz)) {
        return false;
    }

    cleanupActiveRequests(manager, false);

    bool added = true;
    if(lookupRecurringRequest(manager, bus, request) == NULL) {
        ActiveDiagnosticRequest* entry = getFreeEntry(manager);
        if(entry != NULL) {
            if(updateRequiredAcceptanceFilters(bus, request)) {
                updateDiagnosticRequestEntry(entry, manager, bus, request, name,
                        waitForMultipleResponses, decoder, callback, frequencyHz);

                char request_string[128] = {0};
                diagnostic_request_to_string(&entry->handle.request, request_string,
                        sizeof(request_string));

                LIST_REMOVE(entry, listEntries);
                debug("Added recurring diagnostic request (freq: %f) on bus %d: %s",
                        frequencyHz, bus->address, request_string);

                TAILQ_INSERT_HEAD(&manager->recurringRequests, entry, queueEntries);
            } else {
                added = false;
            }
        } else {
            added = false;
        }
    } else {
        debug("Can't add request, one already exists with same key");
        added = false;
    }
    return added;
}

bool openxc::diagnostics::addRecurringRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, float frequencyHz) {
    return addRecurringRequest(manager, bus, request, name,
            waitForMultipleResponses, NULL, NULL, frequencyHz);
}

bool openxc::diagnostics::addRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses) {
    return addRequest(manager, bus, request, name,
            waitForMultipleResponses, NULL, NULL);
}

bool openxc::diagnostics::addRecurringRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, float frequencyHz) {
    return addRecurringRequest(manager, bus, request, NULL, false, frequencyHz);
}

bool openxc::diagnostics::addRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request) {
    return addRequest(manager, bus, request, NULL, false, NULL, NULL);
}

/* Private: After checking for a proper CAN bus and the necessary write
 * permissions, process the requested command.
 */
static bool handleAuthorizedCommand(DiagnosticsManager* manager,
        CanBus* bus, openxc_ControlCommand* command) {
    openxc_DiagnosticControlCommand* diagControlCommand =
            &command->diagnostic_request;
    openxc_DiagnosticRequest* commandRequest = &diagControlCommand->request;
    DiagnosticRequest request = {
        arbitration_id: commandRequest->message_id,
        mode: uint8_t(commandRequest->mode),
    };

    if(commandRequest->payload.size > 0) {
        request.payload_length = commandRequest->payload.size;
        memcpy(request.payload, commandRequest->payload.bytes,
                request.payload_length);
    }

    if(commandRequest->pid != 0) {
        request.has_pid = true;
        request.pid = commandRequest->pid;
    }

    DiagnosticResponseDecoder decoder = NULL;
    if(commandRequest->decoded_type != openxc_DiagnosticRequest_DecodedType_UNUSED) {
        switch(commandRequest->decoded_type) {
            case openxc_DiagnosticRequest_DecodedType_NONE:
                decoder = passthroughDecoder;
                break;
            case openxc_DiagnosticRequest_DecodedType_OBD2:
                decoder = obd2::handleObd2Pid;
                break;
	    default:
		decoder = NULL;
		break;
        }
    } else if(obd2::isObd2Request(&request)) {
        decoder = obd2::handleObd2Pid;
    }

    bool multipleResponses = commandRequest->message_id ==
            OBD2_FUNCTIONAL_BROADCAST_ID;
    if(commandRequest->multiple_responses == true) {
        multipleResponses = commandRequest->multiple_responses;
    }

    bool status = true;
    if(diagControlCommand->action == openxc_DiagnosticControlCommand_Action_ADD) {
        if(commandRequest->frequency != 0.0) {
            status = addRecurringRequest(manager, bus, &request,
                    (strlen(commandRequest->name) > 0) ?
                            commandRequest->name : NULL,
                    multipleResponses,
                    decoder,
                    NULL,
                    commandRequest->frequency);
        } else {
            status = addRequest(manager, bus, &request,
                    (strlen(commandRequest->name) > 0) ?
                            commandRequest->name : NULL,
                    multipleResponses,
                    decoder,
                    NULL);
        }
    } else if(diagControlCommand->action == openxc_DiagnosticControlCommand_Action_CANCEL) {
        status = cancelRecurringRequest(manager, bus, &request);
    }
    return status;
}

bool openxc::diagnostics::handleDiagnosticCommand(
        DiagnosticsManager* manager, openxc_ControlCommand* command) {
    bool status = true;
        openxc_DiagnosticRequest* commandRequest =
                &command->diagnostic_request.request;
        if((commandRequest->message_id != 0) && (commandRequest->mode != 0)) {
            CanBus* bus = NULL;
            if(commandRequest->bus != 0) {
                bus = lookupBus(commandRequest->bus, getCanBuses(),
                        getCanBusCount());
            } else if(getCanBusCount() > 0) {
                bus = &getCanBuses()[0];
                debug("No bus specified for diagnostic request, "
                        "using first active: %d", bus->address);
            }

            if(getConfiguration()->emulatedData){
                    //Checking to see if the message ID is in the "standard" OBD range (7DF or 7E0->7E7)
                    //See: https://en.wikipedia.org/wiki/OBD-II_PIDs#CAN_.2811-bit.29_bus_format
                    if(commandRequest->message_id >= 0x7DF && commandRequest->message_id <= 0x7E7)
                    {
                        openxc_VehicleMessage message = openxc_VehicleMessage();	// Zero fill
                        message.type = openxc_VehicleMessage_Type_DIAGNOSTIC;
                        message.diagnostic_response = {0};
                        message.diagnostic_response.bus = bus->address;
                        //message.diagnostic_response.has_message_id = true;
                        //7DF should respond with a random message id between 7e8 and 7ef
                        //7E0 through 7E7 should respond with a id that is 8 higher (7E0->7E8)
                        if(commandRequest->message_id == 0x7DF)
                        {
                            message.diagnostic_response.message_id = rand()%(0x7EF-0x7E8 + 1) + 0x7E8;
                        }
                        else if(commandRequest->message_id >= 0x7E0 && commandRequest->message_id <= 0x7E7)
                        {
                            message.diagnostic_response.message_id = commandRequest->message_id + 8;
                        }
                        message.diagnostic_response.mode = commandRequest->mode;
                        if(commandRequest->pid != 0)
                        {
                            message.diagnostic_response.pid = commandRequest->pid;
                        }

                        openxc_DynamicField value = openxc_DynamicField();	// Zero fill
                        value.type = openxc_DynamicField_Type_NUM;
                        value.numeric_value = rand() % 100;
                        message.diagnostic_response.value = value;

                        debug("Response message id: %d", message.diagnostic_response.message_id);
                        pipeline::publish(&message, &getConfiguration()->pipeline);
                    }
                    else //If it's outside the range, the command_request will return false
                    {
                        debug("Sent message ID is outside the valid range for emulator (7DF to 7E7)");
                        status=false;
                    }
                }
            else
            {
                if(bus == NULL) {
                    debug("No active bus to send diagnostic request");
                    status = false;
                } else if(bus->rawWritable) {
                        status = handleAuthorizedCommand(manager, bus, command);
                } else {
                    debug("Raw CAN writes not allowed for bus %d", bus->address);
                    status = false;
                }
            }

        } else {
            debug("Diagnostic requests need at least an arb. ID and mode");
            status = false;
        }
    return status;
}

void openxc::diagnostics::passthroughDecoder(
        const DiagnosticResponse* response, float parsed_payload, char* str_buf, int buf_size) {
    if (response->multi_frame) {
        snprintf(str_buf, buf_size, "%s", response->payload);
    } else {
        snprintf(str_buf, buf_size, "%f", parsed_payload);
    }
}
