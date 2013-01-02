#ifndef __CANWRITE_LPC17XX__
#define __CANWRITE_LPC17XX__

#include "canutil.h"

/* Public: Write a CAN message with the given data and node ID to the bus.
 *
 * bus - The CAN bus to send the message on.
 * request - the CanMessage requested to send.
 *
 * Returns true if the message was sent successfully.
 */
bool sendCanMessage(CanBus* bus, CanMessage request);

#endif // __CANWRITE_LPC17XX__
