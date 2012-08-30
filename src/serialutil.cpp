#include "serialutil.h"
#include "buffers.h"
#include "log.h"

void sendMessage(SerialDevice* device, uint8_t* message, int messageSize) {
    for(int i = 0; i < messageSize; i++) {
        if(!QUEUE_PUSH(uint8_t, &device->sendQueue, (uint8_t)message[i])) {
            debug("Dropped incoming CAN message -- send queue full for serial");
            return;
        }
    }
    QUEUE_PUSH(uint8_t, &device->sendQueue, (uint8_t)'\n');
}
