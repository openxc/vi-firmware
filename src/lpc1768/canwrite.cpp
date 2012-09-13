#ifdef __LPC17XX__

#include "bitfield.h"
#include "canutil.h"
#include "canwrite.h"
#include "lpc17xx_can.h"
#include "log.h"
#include <stdbool.h>

void copyToMessageBuffer(uint64_t source, uint8_t* a, uint8_t* b) {
    for(int i = 0, j = 4; i < 3 && j < 8; i++, j++) {
        a[i] = nthByte(source, i);
        b[i] = nthByte(source, j);
    }
}

/* Public: Write a CAN message with the given data and node ID to the bus.
 *
 * The CAN module has an 8 message buffer and sends messages in FIFO order. If
 * the buffer is full, this function will return false and the message will not
 * be sent.
 *
 * bus - The CAN bus to send the message on.
 * request - the CanMessage requested to send.
 *
 * Returns true if the message was sent successfully.
 */
bool sendCanMessage(CanBus* bus, CanMessage request) {
    CAN_MSG_Type message;
    message.id =  request.id;
    message.len = 8;
    message.type = DATA_FRAME;
    message.format = STD_ID_FORMAT;
    copyToMessageBuffer(request.data, message.dataA, message.dataB);

    return CAN_SendMsg(bus->controller, &message) == SUCCESS;
}

#endif // __LPC17XX__
