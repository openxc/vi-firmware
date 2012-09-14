#include "bitfield.h"
#include "canwrite.h"
#include "log.h"

void copyToMessageBuffer(uint64_t source, uint8_t* destination) {
    for(int i = 0; i < 8; i++) {
        destination[i] = nthByte(source, i);
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
    CAN::TxMessageBuffer* message = bus->controller->getTxMessageBuffer(
            CAN::CHANNEL0);
    if (message != NULL) {
        message->messageWord[0] = 0;
        message->messageWord[1] = 0;
        message->messageWord[2] = 0;
        message->messageWord[3] = 0;

        message->msgSID.SID = request.id;
        message->msgEID.IDE = 0;
        message->msgEID.DLC = 8;
        memset(message->data, 0, 8);
        copyToMessageBuffer(request.data, message->data);

        // Mark message as ready to be processed
        bus->controller->updateChannel(CAN::CHANNEL0);
        bus->controller->flushTxChannel(CAN::CHANNEL0);
        return true;
    } else {
        debug("Unable to get TX message area");
    }
    return false;
}
