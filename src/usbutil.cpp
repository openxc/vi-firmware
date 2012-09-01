#include "usbutil.h"
#include "log.h"

void sendMessage(UsbDevice* usbDevice, uint8_t* message, int messageSize) {
    if(queue_available(&usbDevice->sendQueue) < messageSize + 1) {
            debug("Dropped incoming CAN message -- send queue full for USB");
            return;
    }

    for(int i = 0; i < messageSize; i++) {
        QUEUE_PUSH(uint8_t, &usbDevice->sendQueue, (uint8_t)message[i]);
    }
    QUEUE_PUSH(uint8_t, &usbDevice->sendQueue, (uint8_t)'\n');
}
