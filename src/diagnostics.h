#ifndef __DIAGNOSTICS_H__
#define __DIAGNOSTICS_H__

#include "util/timer.h"
#include <can/canutil.h>
#include <obd2/obd2.h>
#include <sys/queue.h>
#include <stdint.h>
#include <stdlib.h>

namespace openxc {
namespace diagnostics {

typedef struct {
    DiagnosticShims shims;
} DiagnosticsManager;

void initialize(DiagnosticsManager* manager);

/* Public:
 *
 * If genericName is null, output will be in raw OBD-II response format.
 *
 * If decoder is null, output will include the raw payload instead of a value.
 */
typedef struct {
    DiagnosticRequestHandle handle;
    const char* genericName;
    const DiagnosticResponseDecoder decoder;
} ActiveDiagnosticRequest;

typedef struct {
    ActiveDiagnosticRequest request;
    openxc::util::time::FrequencyClock frequencyClock;
} RecurringDiagnosticRequest;

void addDiagnosticRequest(DiagnosticRequest* request, const char* genericName,
        const DiagnosticResponseDecoder decoder);

void addRecurringDiagnosticRequest(DiagnosticRequest* request,
        const char* genericName, const DiagnosticResponseDecoder decoder);

void sendRequests(DiagnosticsManager* manager);

void receiveCanMessage(DiagnosticsManager* manager, CanMessage* message);

// TODO need an internal linked list (static allocation, please) of active
// requests and another list of active, recurring requests.

} // namespace diagnnostics
} // namespace openxc

#endif // __DIAGNOSTICS_H__
