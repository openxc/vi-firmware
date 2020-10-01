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
using openxc::can::read::publishVehicleMessage;
using openxc::pipeline::Pipeline;
using openxc::pipeline::MessageClass;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::config::getConfiguration;

namespace time = openxc::util::time;
namespace pipeline = openxc::pipeline;
namespace obd2 = openxc::diagnostics::obd2;

//  receiveCanMessage - called from vi_firmware every time a CanMessage
//                  is QUEUE_POP from the busses' receiveQueue
// MULTIFRAME  0       The Old way Before 2020 (no multiframe messages)
// MULTIFRAME  1       Multi-frame stitched message feature
//
#define MULTIFRAME  1

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
        cleanupActiveRequests(manager, true);
    }

    TAILQ_INIT(&manager->recurringRequests);
    LIST_INIT(&manager->nonrecurringRequests);
    LIST_INIT(&manager->freeRequestEntries);

    for(int i = 0; i < MAX_SIMULTANEOUS_DIAG_REQUESTS; i++) {
        LIST_INSERT_HEAD(&manager->freeRequestEntries,
                &manager->requestListEntries[i], listEntries);
    }
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
    message.diagnostic_response.pid = response->pid;
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

const bool originalStitchAlgo = false;
static int prevFrame = -1;
static openxc_VehicleMessage wrapDiagnosticStitchResponseWithSabot(CanBus* bus,
        const ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response, openxc_DynamicField value) {
    openxc_VehicleMessage message = openxc_VehicleMessage();        // Zero fill
    message.type = openxc_VehicleMessage_Type_DIAGNOSTIC_STITCH;
    message.diagnostic_stitch_response = {0};
    message.diagnostic_stitch_response.bus = bus->address;

    if(request->arbitration_id != OBD2_FUNCTIONAL_BROADCAST_ID) {
        message.diagnostic_stitch_response.message_id = response->arbitration_id
            - DIAGNOSTIC_RESPONSE_ARBITRATION_ID_OFFSET;
    } else {
        // must preserve responding arb ID for responses to functional broadcast
        // requests, as they are the actual module address and not just arb ID +
        // 8.
        message.diagnostic_stitch_response.message_id = response->arbitration_id;
    }

    message.diagnostic_stitch_response.mode = response->mode;
    message.diagnostic_stitch_response.pid = response->pid;
    message.diagnostic_stitch_response.success = response->success;
    message.diagnostic_stitch_response.negative_response_code =
            response->negative_response_code;

    message.diagnostic_stitch_response.frame = prevFrame + 1;
    if (response->completed) {
        message.diagnostic_stitch_response.frame = -1;     // Marks the last frame in the response
    } else {
        message.diagnostic_stitch_response.success = true;
    }
    prevFrame = message.diagnostic_stitch_response.frame;

    if(response->payload_length > 0) {
        if (request->decoder != NULL)  {
            message.diagnostic_stitch_response.value = value;
        } else {
            memcpy(message.diagnostic_stitch_response.payload.bytes, response->payload,
                    response->payload_length);
            message.diagnostic_stitch_response.payload.size = response->payload_length;
        }
    }
    return message;
}

#if (MULTIFRAME != 0)
// The next 2 methods are the origial way that multi-frame
// stitching was achieved.  It has been replaced with a new
// openxc.proto file and uses a new message type.
// These are here for reference until we know the new messages are working
// 6/15/2020
const int MAX_MULTI_FRAME_MESSAGE_SIZE = 300;

static void sendPartialMessage(long timestamp,
                                int frame,
                                int message_id,
                                int bus,
                                int total_size,
                                int mode,
                                int pid,
                                int value,
                                int negative_response_code,
                                const char *payload,
                                int payload_size,
                                Pipeline* pipeline) {

    char messageBuffer[MAX_MULTI_FRAME_MESSAGE_SIZE];
            
    // Manually form the message that is going out.

    int numWritten = snprintf(messageBuffer,
            MAX_MULTI_FRAME_MESSAGE_SIZE,
            "{\"timestamp\":%ld,\"frame\":%d,\"message_id\":%d,\"bus\":%d,\"total_size\":%d,\"mode\":%d,\"pid\":%d,\"value\":%d",
            timestamp,
            frame,
            message_id+8,
            bus,
            total_size,
            mode,
            pid,
            value);
    
    if (negative_response_code != 0) {  // Is there a failure code?
        numWritten += snprintf(messageBuffer+numWritten,
                            MAX_MULTI_FRAME_MESSAGE_SIZE-numWritten,
                            ",\"success\":false,\"negative_response_code\":%d",
                            negative_response_code);
    } else {    // Success
        numWritten += snprintf(messageBuffer+numWritten,
                            MAX_MULTI_FRAME_MESSAGE_SIZE-numWritten,
                            ",\"success\":true");
    }

    numWritten += snprintf(messageBuffer+numWritten,
                            MAX_MULTI_FRAME_MESSAGE_SIZE-numWritten,
                            ",\"payload\":\"0x");

    for(int index=0; (index<payload_size) && (numWritten < MAX_MULTI_FRAME_MESSAGE_SIZE); index++) {
        messageBuffer[numWritten++]=(((payload[index]>>4) & 0x0f) > 9) ? ((payload[index]>>4) & 0x0f) + 'a' - 10 : ((payload[index]>>4) & 0x0f) + '0';
        messageBuffer[numWritten++]=((payload[index]&0xf) > 9) ? (payload[index]&0x0f) + 'a' - 10 : (payload[index]&0xf) + '0';
    }
    numWritten += snprintf(messageBuffer+numWritten,
                            MAX_MULTI_FRAME_MESSAGE_SIZE-numWritten,
                            "\"}");

    int messageLen = strlen(messageBuffer) +1;
    pipeline::sendMessage(pipeline,
        (uint8_t*)messageBuffer, messageLen, MessageClass::SIMPLE);
}


// relayPartialFrame - Send the partial frame to the mobile device/web

static void relayPartialFrame(DiagnosticsManager* manager,  // Only need for the callback
        ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response, 
        Pipeline* pipeline) {
        
        int frame = prevFrame + 1;
        if (response->completed) {
            frame = -1;     // Marks the last frame in the response
        }
        prevFrame = frame;

        // see wrapDiagnosticResponseWithSabot
        sendPartialMessage(00,                                  //     long timestamp,
                           frame,                               //     int frame
                           response->arbitration_id,            //     int message_id,
                           request->bus->address,               //     int bus,
                           0,                                   //     int total_size,
                           response->mode,                      //     int mode,
                           response->pid,                       //     int pid,
                           0,                                   //     int value, - when the payload is a bitfield or numeric - parsed value
                           response->negative_response_code,    //     int negative_response_code
                           (char *)response->payload,           //     char *payload
                           response->payload_length,
                           pipeline);

        if (response->completed && (request->callback != NULL)) {
            request->callback(manager, request, response, diagnostic_payload_to_integer(response));
        }
}
#endif

const int VIN_STORAGE_LENGTH = VIN_LENGTH+1;  // 17 characters + 1 pad
const int VIN_SNIPPET_LENGTH = 6;   // 6 VIN characters per CAN msg
unsigned char vinBuffer[VIN_STORAGE_LENGTH] = {0};
bool vinComplete = false;
bool vinCommandInProgress = false;

bool openxc::diagnostics::haveVINfromCan() {
    return vinComplete;
}

unsigned char *openxc::diagnostics::getVIN() {
    return vinBuffer;
}

void openxc::diagnostics::setVinCommandInProgress(bool vinInProgress) {
    vinCommandInProgress = vinInProgress;
}

void openxc::diagnostics::filterForVIN(CanMessage* message) {

    if ((message->id == 0x40a) &&
        (message->data[0] == 0xc1) && 
        (message->data[1] <= 0x02)) {

        // gja debug here!
        // char buffer[26];
        // buffer[0] = '>';
        // int indx=1;
        // for(int cnt=0; cnt<message->length; cnt++) {
        //     int c = (message->data[cnt] & 0xf0) >> 4;
        //     buffer[indx++] = (c > 9) ? (c - 10 + 'a') : c + '0';
        //     c = (message->data[cnt] & 0x0f);
        //     buffer[indx++] = (c > 9) ? (c - 10 + 'a') : c + '0';
        //     buffer[indx++] = ' ';
        //     buffer[indx] = 0;
        // }
        // debug(buffer);
        // gja debug above here!

        int index = message->data[1] * VIN_SNIPPET_LENGTH;
        for(int cnt=2; cnt<8; cnt++, index++) {
            vinBuffer[index] = message->data[cnt];
        }

        bool emptySpace = false;
        for (int cnt=0; cnt<VIN_LENGTH; cnt++) {
            if (vinBuffer[cnt] == 0) emptySpace = true;
        }
        vinComplete = !emptySpace;
    }
}

void dumpNum(int);
bool vinCommandFailed = false;
int selector = -1;

void openxc::diagnostics::checkForVinCommand(CanMessage *message) {

    // Step 1: Check to see if get vin command is in progrees
    debug("diag:checkForVinCommand");
    dumpNum(message->id);
    if(message->id != 2024) {
        return;
    }
    char buffer[18];
    int bufferindex = 0;
    // Step 2: unpackage Can data into VIN buffer
    if (message->length > 2) {
        for(int index=0; index<message->length; index++) {
            int high = ((message->data[index] >> 4) & 0xf);
            int letter = (high > 9) ? high - 10 + 'A' : high + '0';
            int low = (message->data[index] & 0xf);
            int letter2 = (low > 9) ? low - 10 + 'A' : low + '0';
            buffer[bufferindex] = letter; bufferindex++;
            buffer[bufferindex] = letter2; bufferindex++;
        }
        buffer[bufferindex] = 0;
        debug(buffer);
    }

    if (selector >= 3) selector = -1;

    selector++;
    if(selector == 0) {
        for(int cnt=5, index=0; cnt<8; cnt++, index++) {
        vinBuffer[index] = message->data[cnt];
        }
    } else if(selector == 1) {
        for(int cnt=1, index=3; cnt<8; cnt++, index++) {
        vinBuffer[index] = message->data[cnt];
        }
    } else {
        for(int cnt=1, index=10; cnt<8; cnt++, index++) {
        vinBuffer[index] = message->data[cnt];
        }
    // Step 3: When all of the VIN data is complete return sendCommandResponse with VIN

        char* vin = (char *)vinBuffer;
        if (strlen(vin) == VIN_LENGTH) {
            openxc::commands::sendCommandResponse(openxc_ControlCommand_Type_GET_VIN, 1, vin, strlen(vin));
            setVinCommandInProgress(false);
    // VIN command completed 
            vinComplete = true;
        } else {
            vin = config::getConfiguration()->dummyVin;
            debug("get_vin Command failed.");
            openxc::commands::sendCommandResponse(openxc_ControlCommand_Type_GET_VIN, 1, vin, strlen(vin));
            setVinCommandInProgress(false);
    // VIN command completed 
            vinComplete = true;
            
        }
    }
}

static void relayDiagnosticResponse(DiagnosticsManager* manager,
        ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response, Pipeline* pipeline,
        bool stitchMessage) {
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
    } else if (stitchMessage) {
        openxc_VehicleMessage message = wrapDiagnosticStitchResponseWithSabot(
                request->bus, request, response, field);
        pipeline::publish(&message, pipeline);

        if (!(response->completed)) {
            return; // Only return if this is not the last partial
                    // Otherwise if this is last we want to invoke the callback
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
void sendCommandResponse(openxc_ControlCommand_Type commandType,
    bool status, char* responseMessage, size_t responseMessageLength);
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
                debug("Raw Message for get_vin");
                debug((const char*)message->data);
                // If this is a VIN command we don't want to send a diagnostic response.
                if(vinCommandInProgress == true) {
                    openxc::diagnostics::checkForVinCommand(message);
                    return;
                }
        if (response.multi_frame) {
#if (MULTIFRAME != 0)
            if (originalStitchAlgo) {
                relayPartialFrame(manager, entry, &response, pipeline);
            } else { 
                relayDiagnosticResponse(manager, entry, &response, pipeline, true);   // Added 6/11/2020
            }
#endif
            if (!response.completed) {
                time::tick(&entry->timeoutClock);
            } else {
#if (MULTIFRAME == 0)
                // This is the pre 2020 Way of sending a Diagnostic Response
                // (all at once)
                relayDiagnosticResponse(manager, entry, &response, pipeline, false);
#endif
            }
        } else if (response.completed && entry->handle.completed) {
            if(entry->handle.success) {
                // Handle Single frame messages here!
                relayDiagnosticResponse(manager, entry, &response, pipeline, false);
            } else {
                debug("Fatal error sending or receiving diagnostic request");
            }
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

    if (commandRequest->pid > 0) {
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

bool openxc::diagnostics::isSupportedMessageID(int requestID)
{
    // ID Within Valid Range (701 - 7F1)
    if (requestID >= 0x701 && requestID <= 0x7F1)
    {
        // Reserved IDs
        if (requestID != 0x703 && requestID != 0x750 && requestID != 0x7B0 && requestID != 0x7D7 && requestID != 0x7F0)
        {
            return true;
        }
        else
        {
            // Request ID = Reserved
            debug("Request ID is reserved and not supported by the emulator! Reserved: 0x703, 0x750, 0x7B0, 0x7D7, 0x7F0");
            return false;
        }
    }
    else
    {
        // ID Outside Valid Range (701 - 7F1)
        debug("Request ID is outside the supported range by the emulator! Range: 0x701 - 0x7F1");
        return false;
    }
}

int openxc::diagnostics::getEmulatedMessageID(int requestID)
{
    if(requestID == 0x7DF)
    {
        // 7E8 <= Response ID <= 7EF
        return rand() % (0x7EF - 0x7E8 + 0x1) + 0x7E8;
    }
    else
    {
        // Response ID = Request ID + 8
        return requestID + 0x8;
    }
}

bool openxc::diagnostics::isSupportedMode(int requestMode)
{
    // Supported Modes
    if (requestMode == 0x1 || requestMode == 0x9 || requestMode == 0x22)
    {
        return true;
    }
    else
    {
        // Unsupported Modes
        debug("Request mode is not supported by the emulator! Supported: 0x1, 0x9, 0x22");
        return false;
    }
}

bool openxc::diagnostics::isSupportedPID(int requestMode, int requestPID)
{
    switch (requestMode)
    {
        case 0x1:
            if (requestPID >= 0x0 && requestPID <= 0xA6)
            {
                return true;
            }
            else
            {
                debug("Mode 0x1 does not support that PID! Range: 0x0 - 0xA6");
            }
            break;
        case 0x9:
            if (requestPID >= 0x0 && requestPID <= 0xB)
            {
                return true;
            }
            else
            {
                debug("Mode 0x2 does not support that PID! Range: 0x0 - 0xB");
            }
            break;
        case 0x22:
            if (requestPID >= 0xDE00 && requestPID <= 0xDEEF)
            {
                return true;
            }
            else
            {
                debug("Mode 0x22 does not support that PID! Range: 0xDE00 - 0xDEEF");
            }
            break;
        default:
            break;
    }
    return false;
}

bool openxc::diagnostics::isStitchPID(int requestMode, int requestPID)
{
#if (MULTIFRAME==1)
    if ((requestMode == 0x22) && 
        ((requestPID == 0xde00) || (requestPID == 0xde01))) {
        return true;
    }
#endif
    return false;
}

bool openxc::diagnostics::generateAndSendEmulatedStitchMessages(int requestMode, int requestPID, Pipeline* pipeline) {
#if (MULTIFRAME!=1)
    return false;
#else

    if (requestPID == 0xde00) {
        char emulPayload[] = {(char)0x62, (char)0xDE, (char)0x00, 
                                (char)0x22, (char)0x2a, (char)0x04};
        emulPayload[4] = rand() % 256;
        emulPayload[5] = rand() % 256;
        sendPartialMessage(0x00000000,      // long timestamp,
                           -1,              // int frame,
                           0x7d8,           // int message_id,
                           0,               // int bus,
                           0,               // int total_size,
                           0x22,            // int mode,
                           0xde00,          // int pid,
                           0,               // int value,
                           0,               // int negative_response_code,
                           emulPayload,     // const char *payload,
                           sizeof(emulPayload), // int payload_size,
                           pipeline);
    } else {
        char emulPayload1[] = {(char)0x62, (char)0xDE, (char)0x00, 
                                (char)0x22, (char)0x2a, (char)0x04};
        char emulPayload2[] = {(char)0x03, (char)0x00, (char)0x0a,
                                (char)0xc8, (char)0x00, (char)0x00,
                                (char)0x8a};
        emulPayload1[5] = rand() % 256;
        for(int cnt=0; cnt<7; cnt++) {
            emulPayload2[cnt] = rand() % 256;
        }

        sendPartialMessage(0x00000000,      // long timestamp,
                           0,               // int frame,  (First Frame)
                           0x7d8,           // int message_id,
                           0,               // int bus,
                           0,               // int total_size,
                           0x22,            // int mode,
                           0xde00,          // int pid,
                           0,               // int value,
                           0,               // int negative_response_code,
                           emulPayload1,     // const char *payload,
                           sizeof(emulPayload1), // int payload_size,
                           pipeline);

        sendPartialMessage(0x00000000,      // long timestamp,
                           -1,               // int frame,  (Last Frame)
                           0x7d8,           // int message_id,
                           0,               // int bus,
                           0,               // int total_size,
                           0x22,            // int mode,
                           0xde00,          // int pid,
                           0,               // int value,
                           0,               // int negative_response_code,
                           emulPayload2,     // const char *payload,
                           sizeof(emulPayload2), // int payload_size,
                           pipeline);
    }
    return true;
#endif
}

void openxc::diagnostics::generateEmulatorPayload(openxc_VehicleMessage* vehicleMessage, bool isSuccess)
{
    vehicleMessage->diagnostic_response.success = isSuccess;
    if (isSuccess)
    {
        openxc_DynamicField value = openxc_DynamicField();
        value.type = openxc_DynamicField_Type_NUM;
        value.numeric_value = rand() % 0x1000;
        vehicleMessage->diagnostic_response.value = value;
    }
    else
    {
        vehicleMessage->diagnostic_response.negative_response_code = rand() % (0xF1 - 0x10 + 0x1) + 0x10;
    }
}

bool openxc::diagnostics::isVINPid(int requestMode, int requestPID)
{
#if (MULTIFRAME==1)
    if ((requestMode == 9) && (requestPID == 2)) {
        return true;
    }
#endif
    return false; 
}

static const char *VINArray[] = {
            "1FDWX3C60C0012685",   // 2012 Ford
            "2FTDX15G0C0010824",   // 1982 Ford F150
            "2FDHF37M0R0011608",   // 1994 Ford F350
            "1FTNX20L71EC79927",   // 2001 Ford F250 Super Duty
            "1FTFX1CF2DFC76209",   // 2013 Ford F150
            "1FDXF46P37EB40030",   // 2007 Ford F450
            "1FTBR10T9GUA26470",   // 1986 Ford Ranger
            "1FTNS2ELXADB03208",   // 2010 Ford E Series Van
            "JH4KA3150HC004866",   // 1987 Acura Legend
            "1G8MC35B38Y119771",   // 2008 Saturn Sky
            "1ZVBP8CH7A5121324",   // 2020 Ford Mustang
            "WD5WD641525381291",   // 2002 Freightliner Sprinter
            "WBANF73576CG65408",   // 2006 BMW 5 Series
            "1FTEX14H0RKA51281",   // 1994 Ford F150
            "1GNEL19X73B130926",   // 2003 Chevrolet Astro
            "1C6RD6KT4CS332867",   // 2012 Dodge Ram 1500
            "3D4GG47B09T581222",   // 2009 Dodge Journey
            "JH4DB1670MS000448",   // 1991 Acura Integra
            "1GNDT13W5R2133070",   // 1994 Chevrolet S10 Blazer
            "1G6CD1184H4323745"    // 1987 Cadillac Deville
};

bool openxc::diagnostics::generateAndSendVINStitchMessages(int messageId, int requestMode, int requestPID, Pipeline* pipeline) {
#if (MULTIFRAME!=1)
    return false;
#endif

    int sampleSize = sizeof(VINArray) / sizeof(VINArray[0]);
    int selection = rand() % sampleSize;
    const int packetSize = 4;       // Send max "packetSize" ascii per message
    int numPackets = (strlen(VINArray[selection]) + packetSize - 1) / packetSize;

    int index = 0;
    for(int count=0; count<numPackets; count++) {
        int remaining = strlen(VINArray[selection]) - index;
        int payloadSize = (packetSize < remaining) ? packetSize : remaining;
        int frame = (count == numPackets - 1) ? -1 : count;
        sendPartialMessage(0x00000000,      // long timestamp,
                           frame,           // int frame,
                           messageId + 8,   // int message_id,
                           0,               // int bus,
                           0,               // int total_size,
                           requestMode,     // int mode,
                           requestPID,      // int pid,
                           0,               // int value,
                           0,               // int negative_response_code,
                           &VINArray[selection][index],     // const char *payload,
                           payloadSize,     // int payload_size,
                           pipeline);
        index += packetSize;
    }

    return true;
}

bool openxc::diagnostics::handleDiagnosticCommand(DiagnosticsManager* manager, openxc_ControlCommand* command)
{
    bool status = true;
    openxc_DiagnosticRequest* commandRequest = &command->diagnostic_request.request;

    if((commandRequest->message_id != 0) && (commandRequest->mode != 0))
    {
        CanBus* bus = NULL;
        if(commandRequest->bus >= 0)
        {
            bus = lookupBus(commandRequest->bus, getCanBuses(), getCanBusCount());
        } 
	    if((bus == NULL) && (commandRequest->bus == 0) && (getCanBusCount() > 0))
        {
            // Could not find a bus of 0 so use the 1st one if one not asked for
            bus = &getCanBuses()[0];
            debug("No bus specified for diagnostic request, using first active: %d", bus->address);
        }

        if(getConfiguration()->emulatedData)
        {
            openxc_VehicleMessage message = openxc_VehicleMessage();
            message.type = openxc_VehicleMessage_Type_DIAGNOSTIC;
            message.diagnostic_response = { 0 };
            message.diagnostic_response.bus = bus->address;

            if (isSupportedMessageID(commandRequest->message_id))
            {
                message.diagnostic_response.message_id = getEmulatedMessageID(commandRequest->message_id);

                if (isSupportedMode(commandRequest->mode))
                {
                    message.diagnostic_response.mode = commandRequest->mode;

                    if (isStitchPID(commandRequest->mode, commandRequest->pid)) {
                        generateAndSendEmulatedStitchMessages(commandRequest->mode, commandRequest->pid,
                                                                &getConfiguration()->pipeline);
                    } else if (isVINPid(commandRequest->mode, commandRequest->pid)) {
                        generateAndSendVINStitchMessages(commandRequest->message_id,
                                                                commandRequest->mode, commandRequest->pid,
                                                                &getConfiguration()->pipeline);
                    }
                    else if (isSupportedPID(commandRequest->mode, commandRequest->pid))
                    {
                        message.diagnostic_response.pid = commandRequest->pid;

                        generateEmulatorPayload(&message, rand() & 1);

                        pipeline::publish(&message, &getConfiguration()->pipeline);
                    }
                    else
                    {
                        status = false;
                    }
                }
                else
                {
                    status = false;
                }
            }
            else
            {
                status = false;
            }
        }
        else
        {
            if (bus == NULL)
            {
                debug("No active bus to send diagnostic request");
                status = false;
            }
            else if (bus->rawWritable)
            {
                status = handleAuthorizedCommand(manager, bus, command);
            }
            else
            {
                debug("Raw CAN writes not allowed for bus %d", bus->address);
                status = false;
            }
        }

    }
    else
    {
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