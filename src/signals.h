#ifndef _SIGNALS_H_
#define _SIGNALS_H_

#include "diagnostics.h"
#include "can/canread.h"
#include "can/canwrite.h"
// Not used directly in this header, but keep it around so handlers don't need
// to include it explicitly.
#include "openxc.pb.h"

namespace openxc {

/* All of the functions in this namespace are declared as weak symbols, and a
 * default no-op implementation is provided in default_signals.cpp. The build
 * will complete successfully without any additional implementation but the VI
 * will not read or send any CAN messages. You must provide your own
 * implementation of these functions in a separate file named signals.cpp, which
 * will override the defaults. Watch out, other filenames may not be built in
 * the correct order and they will not override the default implementations!
 */
namespace signals {

/** Public: Return the currently active CAN configuration. */
const CanMessageSet* getActiveMessageSet() __attribute__((weak));

/** Public: Retrive a list of all possible CAN configurations.
 *
 * Returns a pointer to an array of all configurations.
 */
const CanMessageSet* getMessageSets() __attribute__((weak));

/** Public: Return the length of the array returned by getMessageSets() */
int getMessageSetCount() __attribute__((weak));

/* Public: Perform any one-time initialization necessary. This is called when
 * the microcontroller first starts.
 *
 * diagnosticsManager - The active DiagnosticManager, for adding default
 *      diagnostic requests. When you add pre-defined, recurring diagnostic
 *      requests to a VI config file, the Python code generator will initialize
 *      them in this function.
 */
void initialize(openxc::diagnostics::DiagnosticsManager* diagnosticsManager) __attribute__((weak));

/* Public: Any additional processing that should happen each time through the
 * main firmware loop, in addition to the built-in CAN message handling. This
 * function is called once at the end of every iteration of the main loop.
 */
void loop() __attribute__((weak));

/* Public: Return the number of CAN buses configured in the active
 * configuration. This is limited to 2, as the hardware controller only has 2
 * CAN channels.
 */
int getCanBusCount() __attribute__((weak));

/* Public: Return an array of all CAN messages to be processed in the active
 * configuration.
 */
const CanMessageDefinition* getMessages() __attribute__((weak));

/* Public: Return an array of all CAN signals to be processed in the active
 * configuration.
 */
const CanSignal* getSignals() __attribute__((weak));

SignalManager* getSignalManagers() __attribute__((weak));
/* Public: Return an array of all OpenXC CAN commands enabled in the active
 * configuration that can write back to CAN with a custom handler.
 *
 * Commands not defined here are handled using a 1-1 mapping from the signals
 * list.
 */
CanCommand* getCommands() __attribute__((weak));

/* Public: Return the length of the array returned by getCommandCount(). */
int getCommandCount() __attribute__((weak));

/* Public: Return the length of the array returned by getSignals(). */
int getSignalCount() __attribute__((weak));

/* Public: Return the length of the array returned by getMessages(). */
int getMessageCount() __attribute__((weak));

/* Public: Return an array of the metadata for the 2 CAN buses you want to
 * monitor. The size of this array is fixed at 2.
 */
CanBus* getCanBuses() __attribute__((weak));

/* Public: Decode CAN signals from raw CAN messages, translate from engineering
 * units to something more human readable, and send the resulting value over USB
 * as an OpenXC-style JSON message.
 *
 * This is the main workhorse function of the VI. Every time a new
 * CAN message is received that matches one of the signals in the list returend
 * by getSignals(), this function is called with the message ID and 64-bit data
 * field.
 *
 * bus - The CAN bus this message was received on.
 * message - The received CAN message.
 */
void decodeCanMessage(openxc::pipeline::Pipeline* pipeline, CanBus* bus, CanMessage* message) __attribute__((weak));

} // namespace signals
} // namespace openxc

#endif // _SIGNALS_H_
