#ifndef _SIGNALS_H_
#define _SIGNALS_H_

#include "canread.h"
#include "canwrite.h"

/* Public: The number of CAN buses to read. This is limited to 2, as the
 * hardware controller only has 2 CAN channels.
 */
int getCanBusCount();

/* Public: Return an array of all CAN signals you are able to process and
 * translate to send over USB.
 */
CanSignal* getSignals();

/* Public: Return an array of all OpenXC CAN commnds you are able to process and
 * write back to CAN with a custom handler. Commands not defined here are
 * handled using a 1-1 mapping from the signals list.
 */
CanCommand* getCommands();

/* Public: Return the length of the array returned by getCommandCount(). */
int getCommandCount();

/* Public: Return the length of the array returned by getSignals(). */
int getSignalCount();

/* Public: Return an array of the metadata for the 2 CAN buses you want to
 * monitor. The size of this array is fixed at 2.
 */
CanBus* getCanBuses();

/* Public: Return the name of the vehicle or architecture this translator is
 * currently programmed for.
 */
const char* getMessageSet();

/* Public: Decode CAN signals from raw CAN messages, translate from engineering
 * units to something more human readable, and send the resulting value over USB
 * as an OpenXC-style JSON message.
 *
 * This is the main workhorse function of the CAN translator. Every time a new
 * CAN message is received that matches one of the signals in the list returend
 * by getSignals(), this function is called with the message ID and 64-bit data
 * field.
 *
 * bus - The CAN bus this message was received on.
 * id - The 11-bit ID of the incoming CAN message.
 * data - The 64-bit data field of the CAN message.
 */
void decodeCanMessage(CanBus* bus, int id, uint64_t data);

/* Public: Initialize an array of the CAN message filters that should be set for
 * the CAN module with the given address.
 *
 * If an array is of length 0, the CAN acceptance filter will be disabled and
 * all CAN messages will be passed through the translation stack.
 *
 * address - The address of the CAN module to retreive the filters for.
 * count - An OUT variable that will be set to the length of the returned
 *         filters array.
 *
 * Returns an array of CanFilters that should be initialized on the CAN
 * module with the given address.
 */
CanFilter* initializeFilters(uint64_t address, int* count);

#endif // _SIGNALS_H_
