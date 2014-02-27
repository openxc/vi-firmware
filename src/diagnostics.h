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
 * parsed_payload - the entire payload of the response parsed as a single
 *      integer, then transformed with the registered factor and offset
 *      to a float.
 */
typedef float (*DiagnosticResponseDecoder)(const DiagnosticResponse* response,
        float parsed_payload);

/* Public:
 *
 * If genericName is null, output will be in raw OBD-II response format.
 *
 * If decoder is null, output will include the raw payload instead of a value.
 */
typedef struct {
    CanBus* bus;
    uint16_t arbitration_id;
    DiagnosticRequestHandle handle;
    char genericName[MAX_GENERIC_NAME_LENGTH];
    bool parsePayload;
    float factor;
    float offset;
    DiagnosticResponseDecoder decoder;
    bool recurring;
    openxc::util::time::FrequencyClock frequencyClock;
    openxc::util::time::FrequencyClock timeoutClock;
} ActiveDiagnosticRequest;

struct DiagnosticRequestListEntry {
    ActiveDiagnosticRequest request;
    TAILQ_ENTRY(DiagnosticRequestListEntry) queueEntries;
    LIST_ENTRY(DiagnosticRequestListEntry) listEntries;
};

LIST_HEAD(DiagnosticRequestList, DiagnosticRequestListEntry);
TAILQ_HEAD(DiagnosticRequestQueue, DiagnosticRequestListEntry);

typedef struct {
    DiagnosticShims shims[MAX_SHIM_COUNT];
    DiagnosticRequestQueue activeRequests;
    DiagnosticRequestList inFlightRequests;
    DiagnosticRequestList freeActiveRequests;
    DiagnosticRequestListEntry requestListEntries[MAX_SIMULTANEOUS_DIAG_REQUESTS];
} DiagnosticsManager;

void initialize(DiagnosticsManager* manager, CanBus* buses, int busCount);

/* Public:
 *
 * frequencyHz - a value of 0 means it's a non-recurring request.
 */
bool addDiagnosticRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request, const char* genericName,
        float factor, float offset, const DiagnosticResponseDecoder decoder,
        const uint8_t frequencyHz);

bool addDiagnosticRequest(DiagnosticsManager* manager, CanBus* bus,
        DiagnosticRequest* request, const uint8_t frequencyHz);

void receiveCanMessage(DiagnosticsManager* manager, CanBus* bus,
        CanMessage* message, openxc::pipeline::Pipeline* pipeline);

void sendRequests(DiagnosticsManager* manager, CanBus* bus);

void loop(DiagnosticsManager* manager);

} // namespace diagnostics
} // namespace openxc

#endif // __DIAGNOSTICS_H__
