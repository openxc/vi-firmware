#ifndef __DIAGNOSTICS_H__
#define __DIAGNOSTICS_H__

#include "util/timer.h"
#include <can/canutil.h>
#include <obd2/obd2.h>
#include <sys/queue.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_SIMULTANEOUS_DIAG_REQUESTS 10
#define MAX_GENERIC_NAME_LENGTH 40

namespace openxc {
namespace diagnostics {

/* Public:
 *
 * If genericName is null, output will be in raw OBD-II response format.
 *
 * If decoder is null, output will include the raw payload instead of a value.
 */
typedef struct {
    DiagnosticRequestHandle handle;
    char genericName[MAX_GENERIC_NAME_LENGTH];
    DiagnosticResponseDecoder decoder;
    bool recurring;
    openxc::util::time::FrequencyClock frequencyClock;
} ActiveDiagnosticRequest;

struct ActiveRequestListEntry {
    ActiveDiagnosticRequest request;
    LIST_ENTRY(ActiveRequestListEntry) entries;
};

LIST_HEAD(ActiveRequestList, ActiveRequestListEntry);

typedef struct {
    DiagnosticShims shims;
    ActiveRequestList activeRequests;
    ActiveRequestList freeActiveRequests;
    ActiveRequestListEntry activeListEntries[MAX_SIMULTANEOUS_DIAG_REQUESTS];
} DiagnosticsManager;

void initialize(DiagnosticsManager* manager);

/* Public:
 *
 * frequencyHz - a value of 0 means it's a non-recurring request.
 */
bool addDiagnosticRequest(DiagnosticsManager* manager,
        DiagnosticRequest* request, const char* genericName,
        const DiagnosticResponseDecoder decoder, const uint8_t frequencyHz);

bool addDiagnosticRequest(DiagnosticsManager* manager,
        DiagnosticRequest* request, const char* genericName,
        const DiagnosticResponseDecoder decoder);

void receiveCanMessage(DiagnosticsManager* manager, CanMessage* message);

void sendRequests(DiagnosticsManager* manager);

} // namespace diagnnostics
} // namespace openxc

#endif // __DIAGNOSTICS_H__
