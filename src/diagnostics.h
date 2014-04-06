#ifndef __DIAGNOSTICS_H__
#define __DIAGNOSTICS_H__

#include <sys/queue.h>
#include "bsd_queue_patch.h"
#include <stdint.h>
#include <stdlib.h>
#include "util/timer.h"
#include "pipeline.h"
#include <can/canutil.h>
#include <uds/uds.h>
#include <openxc.pb.h>

#define MAX_SHIM_COUNT 2
#define MAX_SIMULTANEOUS_DIAG_REQUESTS 20
#define MAX_GENERIC_NAME_LENGTH 40

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

typedef void (*DiagnosticResponseCallback)(
            struct DiagnosticsManager* manager,
            const struct ActiveDiagnosticRequest* request,
            const DiagnosticResponse* response,
            float parsed_payload);

/* Public:
 *
 * If name is null, output will be in raw OBD-II response format.
 *
 * If decoder is null, output will include the raw payload instead of a value.
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

void initialize(DiagnosticsManager* manager, CanBus* buses, int busCount,
        uint8_t obd2BusAddress);

void reset(DiagnosticsManager* manager);

/* Public:
 *
 * frequencyHz - a value of 0 means it's a non-recurring request.
 */
bool addRecurringRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, const DiagnosticResponseDecoder decoder,
        const DiagnosticResponseCallback callback, float frequencyHz);

bool addRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, const DiagnosticResponseDecoder decoder,
        const DiagnosticResponseCallback callback);

bool addRecurringRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses, float frequency);

bool addRequest(DiagnosticsManager* manager,
        CanBus* bus, DiagnosticRequest* request, const char* name,
        bool waitForMultipleResponses);

bool addRecurringRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request, float frequencyHz);

bool addRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request);

bool cancelRecurringRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request);

void receiveCanMessage(DiagnosticsManager* manager, CanBus* bus,
        CanMessage* message, openxc::pipeline::Pipeline* pipeline);

void sendRequests(DiagnosticsManager* manager, CanBus* bus);

/* Public: Handle an incoming command, which could be from a USB control
 * transfer or a deserilaized command from UART.
 *
 *  - VERSION_CONTROL_COMMAND - return the version of the firmware as a string,
 *      including the vehicle it is built to translate.
 */
bool handleDiagnosticCommand(DiagnosticsManager* manager,
        openxc_ControlCommand* command);

float passthroughDecoder(const DiagnosticResponse* response,
        float parsed_payload);

} // namespace diagnostics
} // namespace openxc

#endif // __DIAGNOSTICS_H__
