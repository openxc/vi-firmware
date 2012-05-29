#include "canwrite_chipkit.h"
#include "canwrite.h"
#include "WProgram.h"

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
        memcpy(message->data, data, 8);
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
    delay(100);
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
