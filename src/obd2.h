#ifndef __OBD2_H__
#define __OBD2_H__

#include <stdint.h>
#include <stdlib.h>
#include "util/timer.h"
#include "diagnostics.h"

namespace openxc {
namespace diagnostics {
namespace obd2 {

/* Public: The main loop for the OBD-II module of the diagnostics system.
 *
 * This function should be called every time through the main event loop. It
 * controls the status of the diagnostics module when using the
 * OBD2_IGNITION_CHECK power mode. It also suspends the diagnostics module
 * (causing requests to stop sending) when the engine and vehicle speed are
 * zero, to avoid keeping the CAN bus alive with recurring requests even after
 * the user leaves the vehicle.
 *
 * If recurring OBD-II requests are enabled, this will also kick off a
 * diagnostic request for supported PIDs when the engine is on or vehicle is
 * in motion. When the supported PIDs are confirmed, a pre-defined set will be
 * added as recurring requests (see obd2.cpp for those predefined PIDs).
 */
void loop(DiagnosticsManager* manager);

/* Public: Initialize the OBD-II module.
 *
 * Kicks off an ignition status check (engine and vehicle speed). Make sure to
 * call loop(...) after initializing this module to continue with normal
 * functionality.
 */
void initialize(DiagnosticsManager* manager);

/* Public: Check if a request is an OBD-II PID request.
 *
 * Returns true if the request is a mode 1  request and it has a 1 byte PID.
 */
bool isObd2Request(DiagnosticRequest* request);

/* Public: Decode the payload of an OBD-II PID.
 *
 * This function matches the type signature for a DiagnosticResponseDecoder, so
 * it can be used as the decoder for a DiagnosticRequest. It returns the decoded
 * value of the PID, using the standard formulas (see
 * http://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_01).
 */
float handleObd2Pid(const DiagnosticResponse* response, float parsedPayload);

} // namespace obd2
} // namespace diagnostics
} // namespace openxc

#endif // __OBD2_H__
