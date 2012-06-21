#ifndef _SIGNALS_H_
#define _SIGNALS_H_

#include "canread_chipkit.h"
#include "canread.h"
#include "canwrite.h"
#include "handlers.h"
#include "shared_handlers.h"

extern CanUsbDevice usbDevice;
extern CAN can1;
extern CAN can2;
extern void handleCan1Interrupt();
extern void handleCan2Interrupt();

/* Public: The number of CAN buses to read. This is limited to 2, as the
 * hardware controller only has 2 CAN channels.
 */
const int CAN_BUS_COUNT = 2;

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

/* Public: Decode CAN signals from raw CAN messages, translate from engineering
 * units to something more human readable, and send the resulting value over USB
 * as an OpenXC-style JSON message.
 *
 * This is the main workhorse function of the CAN translator. Every time a new
 * CAN message is received that matches one of the signals in the list returend
 * by getSignals(), this function is called with the message ID and 64-bit data
 * field.
 *
 * id - The node ID of the incoming CAN message.
 * data - The 64-bit data field of the CAN message.
 */
void decodeCanMessage(int id, uint8_t* data);

/* Public: Initialize an array of the CAN message filter masks that should be
 * set for the CAN module with the given address.
 *
 * address - The address of the CAN module to retreive the filter masks for.
 * count - An OUT variable that will be set to the length of the returned filter
 *         mask array.
 * TODO now that we have the CanBus struct, we can probably combine the filter
 * and filter mask initialization with that. We could have them defined
 * statically instead of requiring these functions to be called.
 *
 * Returns an array of CanFilterMasks that should be initialized on the CAN
 * module with the given address.
 */
CanFilterMask* initializeFilterMasks(uint64_t address, int* count);

/* Public: Initialize an array of the CAN message filters that should be set for
 * the CAN module with the given address.
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
