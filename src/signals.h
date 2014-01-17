#ifndef _SIGNALS_H_
#define _SIGNALS_H_

#include "can/canread.h"
#include "can/canwrite.h"

namespace openxc {
namespace signals {

/** Public: Return the currently active CAN configuration. */
CanMessageSet* getActiveMessageSet();

/** Public: Retrive a list of all possible CAN configurations.
 *
 * Returns a pointer to an array of all configurations.
 */
CanMessageSet* getMessageSets();

/** Public: Return the length of the array returned by getMessageSets() */
int getMessageSetCount();

/* Public: Perform any one-time initialization necessary. This is called when
 * the microcontroller first starts.
 *
 * TODO should this also be called when the configuration is switched?
 */
void initialize();

/* Public: Any additional processing that should happen each time through the
 * main firmware loop, in addition to the built-in CAN message handling. This
 * function is called once at the end of every iteration of the main loop.
 */
void loop();

/* Public: Return the number of CAN buses configured in the active
 * configuration. This is limited to 2, as the hardware controller only has 2
 * CAN channels.
 */
int getCanBusCount();

/* Public: Return an array of all CAN messages to be processed in the active
 * configuration.
 */
CanMessageDefinition* getMessages();

/* Public: Return an array of all CAN signals to be processed in the active
 * configuration.
 */
CanSignal* getSignals();

/* Public: Return an array of all OpenXC CAN commands enabled in the active
 * configuration that can write back to CAN with a custom handler.
 *
 * Commands not defined here are handled using a 1-1 mapping from the signals
 * list.
 */
CanCommand* getCommands();

/* Public: Return the length of the array returned by getCommandCount(). */
int getCommandCount();

/* Public: Return the length of the array returned by getSignals(). */
int getSignalCount();

/* Public: Return the length of the array returned by getMessages(). */
int getMessageCount();

/* Public: Return an array of the metadata for the 2 CAN buses you want to
 * monitor. The size of this array is fixed at 2.
 */
CanBus* getCanBuses();

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
 * id - The 11-bit ID of the incoming CAN message.
 * data - The 64-bit data field of the CAN message.
 */
void decodeCanMessage(openxc::pipeline::Pipeline* pipeline, CanBus* bus, int id, uint64_t data);

} // namespace signals
} // namespace openxc

#endif // _SIGNALS_H_
