#include "listener.h"
#include "log.h"
#include "buffers.h"

void conditionalEnqueue(QUEUE_TYPE(uint8_t)* queue, uint8_t* message,
        int messageSize) {
    if(queue_available(queue) < messageSize + 1) {
            debug("Dropped incoming CAN message -- send queue full for USB");
            return;
    }

    for(int i = 0; i < messageSize; i++) {
        QUEUE_PUSH(uint8_t, queue, (uint8_t)message[i]);
    }
    QUEUE_PUSH(uint8_t, queue, (uint8_t)'\n');
}

void sendMessage(Listener* listener, uint8_t* message, int messageSize) {
    conditionalEnqueue(&listener->usb->sendQueue, message, messageSize);
    conditionalEnqueue(&listener->serial->sendQueue, message, messageSize);
}

void processListenerQueues(Listener* listener) {
    processInputQueue(listener->usb);
    processInputQueue(listener->serial);
}
