#ifndef __DIAGNOSTICS_H__
#define __DIAGNOSTICS_H__

#include <sys/queue.h>
#include <stdint.h>
#include <stdlib.h>

#include "bsd_queue_patch.h"
#include "pipeline.h"
#include "can/canutil.h"
#include <uds/uds.h>
#include "openxc.pb.h"

/* Private: The maximum number of simultanous diagnostic requests. Increasing
 * this number will use more memory on the stack.
 */
#define MAX_SIMULTANEOUS_DIAG_REQUESTS 20

/* Private: The maximum length for a human-readable name for a diagnostic
 * response.
 */
#define MAX_GENERIC_NAME_LENGTH 40

/* Private: Each CAN bus needs its own set of shim functions, so this should
 * match the maximum CAN controller count.
 */
#define MAX_SHIM_COUNT 2

namespace openxc {
namespace diagnostics {

/* Public: The signature for an optional function that can apply the neccessary
 * formula to translate the binary payload into meaningful data.
 *
 * response - the received DiagnosticResponse (the data is in response.payload,
 *      a byte array). This is most often used when the byte order is
 *      signiticant, i.e. with many OBD-II PID formulas.
 * parsed_payload - the entire payload of the response parsed as an int.
 */
typedef float (*DiagnosticResponseDecoder)(const DiagnosticResponse* response,
        float parsed_payload);

/* Public: The signature for an optional function to handle a new diagnostic
 * response.
 *
 * manager - The DiagnosticsManager providing this response.
 * request - The original diagnostic request.
 * response - The response object that was just received.
 * parsed_payload - The payload of the response, parsed as a float.
 */
typedef void (*DiagnosticResponseCallback)(
        struct DiagnosticsManager* manager,
        const struct ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response,
        float parsed_payload);

/* Private: An active diagnostic request, either recurring or one-time.
 *
 * bus - The CAN bus this request should be made on, or is currently in flight
 *      on.
 * arbitration_id - The arbitration ID (aka message ID) for the request.
 * handle - (Private) A handle for the request to keep track of it between
 *      sending the frames of the request and receiving all frames of the
 *      response.
 * name - An optional human readable name this response, to be used when
 *      publishing received responses. If the name is NULL, the published output
 *      will use the raw OBD-II response format.
 * decoder - An optional DiagnosticResponseDecoder to parse the payload of
 *      responses to this request. If the decoder is NULL, the output will
 *      include the raw payload instead of a parsed value.
 * callback - An optional DiagnosticResponseCallback to be notified whenever a
 *      response is received for this request.
 * recurring - If true, this is a recurring request and it will remain as active
 *      until explicitly cancelled. The frequencyClock attribute controls how
 *      often a recurrin request is made.
 * waitForMultipleResponses - False by default, when any response is received
 *      for a request it will be removed from the active list. If true, the
 *      request will remain active until the timeout clock expires, to allow it
 *      to receive multiple response (e.g. to a functional broadcast request).
 *
 * Really Private:
 *
 * inFlight - True if the request has been sent and we are waiting for a
 *      response.
 * frequencyClock - A FrequencyClock struct to control the send rate for a
 *      recurring request. If the request is not reecurring, this attribute is
 *      not used.
 * timeoutClock - A FrequencyClock struct to monitor how long it's been since
 *      this request was sent.
 * queueEntries - Internal data structure reference for when this request is in
 *      the recurring requests queue.
 * listEntries - Internal data structure reference for when this request is in
 *      the non-recurring requests list or free list.
 */
struct ActiveDiagnosticRequest {
    CanBus* bus;
    uint32_t arbitration_id;
    DiagnosticRequestHandle handle;
    char name[MAX_GENERIC_NAME_LENGTH];
    DiagnosticResponseDecoder decoder;
    DiagnosticResponseCallback callback;
    bool recurring;
    bool waitForMultipleResponses;
    bool inFlight;
    openxc::util::time::FrequencyClock frequencyClock;
    openxc::util::time::FrequencyClock timeoutClock;

    TAILQ_ENTRY(ActiveDiagnosticRequest) queueEntries;
    LIST_ENTRY(ActiveDiagnosticRequest) listEntries;
};
typedef struct ActiveDiagnosticRequest ActiveDiagnosticRequest;

LIST_HEAD(DiagnosticRequestList, ActiveDiagnosticRequest);
TAILQ_HEAD(DiagnosticRequestQueue, ActiveDiagnosticRequest);

/* Public: The core structure for running the diagnostics module on the VI.
 *
 * This stores details about the active requests and shims required to connect
 * the diagnostics library to the VI's CAN peripheral.
 *
 * shims - An array of shim functions for each CAN bus that plug the diagnostics
 *      library (uds-c) into the VI's CAN peripheral.
 * obd2Bus - A reference to the CAN bus that should be used for all standard
 *      OBD-II requests, if the bus is not explicitly spcified in the request.
 *      If NULL, all requests require an explicit bus.
 *
 * Private:
 *
 * recurringRequests - A queue of active, recurring diagnostic requests. When a
 *      response is received for a recurring request or it times out, it is
 *      popped from the queue and pushed onto the back.
 * nonrecurringRequests - A list of active one-time diagnostic requests. When a
 *      response is received for a non-recurring request or it times out, it is
 *      removed from this list and placed back in the free list.
 * freeRequestEntries - A list of all available slots for active diagnostic
 *      requests. This free list is backed by statically allocated entries in
 *      the requestListEntries attribute.
 * requestListEntries - Static allocation for all active diagnostic requests.
 * initialized - True if the DiagnosticsManager has been initialized.
 */
struct DiagnosticsManager {
    DiagnosticShims shims[MAX_SHIM_COUNT];
    CanBus* obd2Bus;
    DiagnosticRequestQueue recurringRequests;
    DiagnosticRequestList nonrecurringRequests;
    DiagnosticRequestList freeRequestEntries;
    ActiveDiagnosticRequest requestListEntries[MAX_SIMULTANEOUS_DIAG_REQUESTS];
    bool initialized;
};
typedef struct DiagnosticsManager DiagnosticsManager;

/* Public: Initialize the diagnostics module, using the given manager to store
 * metadata.
 *
 * manager - The manager object that stores all runtime information about the
 *      module (this must remain in memory somewhere).
 * buses - An array of all active CAN buses.
 * busCount - The length of the buses array.
 * obd2BusAddress - If 0, OBD-II requests will not be sent. Otherwise, they will
 *      be sent on the bus with this controller address (i.e. 1 or 2).
 */
void initialize(DiagnosticsManager* manager, CanBus* buses, int busCount,
        uint8_t obd2BusAddress);

/* Public: Cancel all active diagnostic requests.
 */
void reset(DiagnosticsManager* manager);

/* Public: Add and send a new recurring diagnostic request.
 *
 * This also adds any neccessary CAN acceptance filters so we can receive the
 * response. If the request is to the functional broadcast ID (0x7df) filters
 * are added for all functional addresses (0x7e8 to 0x7f0).
 *
 * At most one recurring request can be active for the same arbitration ID, mode
 * and (if set) PID on the same bus at one time. If you try and call
 * addRecurringRequest with the same key, it will return an error.
 *
 * Example:
 *
 *     // Creating a functional broadcast, mode 1 request for PID 2.
 *     DiagnosticRequest request = {
 *         arbitration_id: 0x7df,
 *         mode: 1,
 *         has_pid: true,
 *         pid: 2
 *     };
 *
 *     // Add a recurring request, to be sent at 1Hz, and published with the
 *     // name "my_pid_request"
 *     addRecurringRequest(&getConfiguration()->diagnosticsManager,
 *          canBus,
 *          &request,
 *          "my_pid_request",
 *          false,
 *          NULL,
 *          NULL,
 *          1);
 *
 * manager - The manager to manage this request.
 * bus - The bus to send the request.
 * request - The parameters for the request.
 * name - An optional human readable name this response, to be used when
 *      publishing received responses. If the name is NULL, the published output
 *      will use the raw OBD-II response format.
 * waitForMultipleResponses - If false, When any response is received
 *      for this request it will be removed from the active list. If true, the
 *      request will remain active until the timeout clock expires, to allow it
 *      to receive multiple response. Functional broadcast requests will always
 *      waint for the timeout, regardless of this parameter.
 * decoder - An optional DiagnosticResponseDecoder to parse the payload of
 *      responses to this request. If the decoder is NULL, the output will
 *      include the raw payload instead of a parsed value.
 * callback - An optional DiagnosticResponseCallback to be notified whenever a
 *      response is received for this request.
 * frequencyHz - The frequency (in Hz) to send the request. A frequency above
 *      MAX_RECURRING_DIAGNOSTIC_FREQUENCY_HZ is not allowed, and will make this
 *      function return false.
 *
 * Returns true if the request was added successfully. Returns false if there
 * wasn't a free active request entry, if the frequency was too high or if the
 * CAN acceptance filters could not be configured,
 */
bool addRecurringRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, const DiagnosticResponseDecoder decoder,
        const DiagnosticResponseCallback callback, float frequencyHz);

/* Public: Add and send a new one-time diagnostic request.
 *
 * A one-time (aka non-recurring) request can existing in parallel with a
 * recurring request for the same PID or mode, that's not a problem.
 *
 * For an example, see the docs for addRecurringRequest. This function is very
 * similar but leaves out the frequencyHz parameter.
 *
 * manager - The manager to manage this request.
 * bus - The bus to send the request.
 * request - The parameters for the request.
 * name - An optional human readable name this response, to be used when
 *      publishing received responses. If the name is NULL, the published output
 *      will use the raw OBD-II response format.
 * waitForMultipleResponses - If false, When any response is received
 *      for this request it will be removed from the active list. If true, the
 *      request will remain active until the timeout clock expires, to allow it
 *      to receive multiple response. Functional broadcast requests will always
 *      waint for the timeout, regardless of this parameter.
 * decoder - An optional DiagnosticResponseDecoder to parse the payload of
 *      responses to this request. If the decoder is NULL, the output will
 *      include the raw payload instead of a parsed value.
 * callback - An optional DiagnosticResponseCallback to be notified whenever a
 *      response is received for this request.
 *
 * Returns true if the request was added successfully. Returns false if there
 * wasn't a free active request entry, if the frequency was too high or if the
 * CAN acceptance filters could not be configured,
 */
bool addRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, const DiagnosticResponseDecoder decoder,
        const DiagnosticResponseCallback callback);

/* Public: A simpler version of the addRecurringRequest function that uses the
 * default response decoder and no response callback.
 */
bool addRecurringRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, float frequency);

/* Public: A simpler version of the addRequest function that uses the
 * default response decoder and no response callback.
 */
bool addRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses);

/* Public: An even simpler version of addRecurringRequest that uses the default
 * decoder, no response callback, no human readable name and finishes after the
 * first response.
 */
bool addRecurringRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request, float frequencyHz);

/* Public: An even simpler version of addRequest that uses the default decoder,
 * no response callback, no human readable name and finishes after the first
 * response.
 */
bool addRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request);

/* Public: Cancel an existing recurring diagnostic request.
 *
 * manager - The manager to cancel the recurring request on.
 * bus - The bus for the recurring request to cancel.
 * request - Match an existing recurring request with the request argument's
 *      bus, arbitration ID, mode and (if set) pid.
 *
 * Returns true if a matching recurring request was found and cancelled.
 */
bool cancelRecurringRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request);

/* Public: Handle a newly received CAN message, checking to see if it is a
 *      response to an active requests.
 *
 * This must be called for every new CAN message that is received. It will match
 * it to any existing requests, relay the response and perform any necessary
 * callbacks.
 *
 * manager - The manager that should receive the CAN message.
 * bus - The bus this message was received from.
 * message - The message received.
 * pipeline - The pipeline to publish any responses.
 */
void receiveCanMessage(DiagnosticsManager* manager, CanBus* bus,
        CanMessage* message, openxc::pipeline::Pipeline* pipeline);

/* Public: Send CAN messages for any active requests that have outstanding CAN
 *      frames that need to be sent.
 *
 * This should be called from the main loop of the firmware in order to handle
 * multi-frame requests as quickly as possible.
 *
 * manager - The manager to send the requests for.
 * bus - The bus to send the requests on.
 */
void sendRequests(DiagnosticsManager* manager, CanBus* bus);

/* Public: Handle an incoming command that claims to be a diagnostic request.
 *
 * This handles requests in the OpenXC message format
 * (https://github.com/openxc/openxc-message-format) to add one-off or recurring
 * diagnostic requests. The request bus must have raw CAN writes enabled, or the
 * request will not be sent (and this function will return false).
 *
 * manager - The manager that should handle this response.
 * command - The command received.
 *
 * Returns true if the command was a diagnostic request, was properly formed
 * and was handled without errors.
 */
bool handleDiagnosticCommand(DiagnosticsManager* manager,
        openxc_ControlCommand* command);

/* Public: A no-op decoder for the payload of a diagnostic response.
 *
 * This is an implementation of DiagnosticResponseDecoder.
 *
 * Returns the already parsed payload with no modifications.
 */
float passthroughDecoder(const DiagnosticResponse* response,
        float parsed_payload);

} // namespace diagnostics
} // namespace openxc

#endif // __DIAGNOSTICS_H__
