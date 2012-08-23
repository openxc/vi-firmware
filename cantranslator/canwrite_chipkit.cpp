#include "canwrite.h"
#include "log.h"

/* Private: Write a CAN message with the given data and node ID to the bus.
 *
 * The CAN module has an 8 message buffer and sends messages in FIFO order. If
 * the buffer is full, this function will return false and the message will not
 * be sent.
 *
 * bus - The CAN bus to send the message on.
 * destination - the destination node ID for the message.
 * data - the data for the message (just 64-bits, not an array or anything).
 *
 * Returns true if the message was sent successfully.
 */
bool sendCanMessage(CAN* bus, uint32_t destination, uint64_t* data) {
    CAN::TxMessageBuffer* message = bus->getTxMessageBuffer(CAN::CHANNEL0);
    if (message != NULL) {
        message->messageWord[0] = 0;
        message->messageWord[1] = 0;
        message->messageWord[2] = 0;
        message->messageWord[3] = 0;

        message->msgSID.SID = destination;
        message->msgEID.IDE = 0;
        message->msgEID.DLC = 8;
        memset(message->data, 0, 8);
        for(int i = 0; i < 8; i++) {
            memcpy(&message->data[i], &((uint8_t*)data)[7 - i], 8);
        }
        debug("Sending message 0x");
        for(int i = 0; i < 8; i++) {
            debug("%x", message->data[i]);
        }
        debug(" to 0x%X", destination);

        // Mark message as ready to be processed
        bus->updateChannel(CAN::CHANNEL0);
        bus->flushTxChannel(CAN::CHANNEL0);
        return true;
    } else {
        debug("Unable to get TX message area");
    }
    return false;
}

bool sendCanSignal(CanSignal* signal, uint64_t data, bool* send) {
    if(send) {
        sendCanMessage(signal->bus->bus, signal->messageId, &data);
        return true;
    } else {
        debug("Not sending requested message %x", signal->messageId);
    }
}

bool sendCanSignal(CanSignal* signal, cJSON* value, CanSignal* signals,
        int signalCount) {
    uint64_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*)
        = signal->writeHandler;
    return sendCanSignal(signal, value, writer, signals, signalCount);
}

bool sendCanSignal(CanSignal* signal, cJSON* value,
        uint64_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*),
        CanSignal* signals, int signalCount) {
    bool send = true;
    if(writer == NULL) {
        if(signal->stateCount > 0) {
            writer = stateWriter;
        } else {
            writer = numberWriter;
        }
    }

    uint64_t data = writer(signal, signals, signalCount, value, &send);
    return sendCanSignal(signal, data, &send);
}
