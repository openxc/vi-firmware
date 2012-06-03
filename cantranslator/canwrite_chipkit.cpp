#include "canwrite_chipkit.h"
#include "canwrite.h"
#include "WProgram.h"

/* Private: Write a CAN message with the given data and node ID to the bus.
 *
 * The CAN module has an 8 message buffer and sends messages in FIFO order. If
 * the buffer is full, this function will fail silently and the message will not
 * be sent. Well, almost silently - it prints something out to the serial
 * terminal.
 *
 * TODO return a boolean indicating if the message was sent or not
 *
 * bus - The CAN bus to send the message on.
 * destination - the destination node ID for the message.
 * data - the data for the message (just 64-bits, not an array or anything).
 */
void sendCanMessage(CAN* bus, uint32_t destination, uint64_t* data) {
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
        Serial.print("Sending message 0x");
        for(int i = 0; i < 8; i++) {
            Serial.print(message->data[i], HEX);
        }
        Serial.print(" to 0x");
        Serial.println(destination, HEX);

        // Mark message as ready to be processed
        bus->updateChannel(CAN::CHANNEL0);
        bus->flushTxChannel(CAN::CHANNEL0);
    } else {
        Serial.println("Unable to get TX message area");
    }
}

void sendCanSignal(CanSignal* signal, cJSON* value,
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
    if(send) {
        sendCanMessage(signal->bus->bus, signal->messageId, &data);
    } else {
        Serial.print("Not sending requested message ");
        Serial.println(signal->messageId, HEX);
    }
}
