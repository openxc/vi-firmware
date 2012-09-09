#include "listener.h"
#include "log.h"
#include "buffers.h"

void conditionalEnqueue(QUEUE_TYPE(uint8_t)* queue, uint8_t* message,
        int messageSize) {
    if(queue_available(queue) < messageSize + 2) {
        debug("Dropped incoming CAN message -- send queue (at %p) full\r\n",
                queue);
        return;
    }

    for(int i = 0; i < messageSize; i++) {
        QUEUE_PUSH(uint8_t, queue, (uint8_t)message[i]);
    }
    QUEUE_PUSH(uint8_t, queue, (uint8_t)'\r');
    QUEUE_PUSH(uint8_t, queue, (uint8_t)'\n');
}

void sendMessage(Listener* listener, uint8_t* message, int messageSize) {
    if(listener->usb->configured) {
        conditionalEnqueue(&listener->usb->sendQueue, message, messageSize);
    } else {
        conditionalEnqueue(&listener->serial->sendQueue, message, messageSize);
    }
}

void processListenerQueues(Listener* listener) {
    // Must always process USB, because this function usually runs the MCU's USB
    // task that handles SETUP and enumeration.
    processInputQueue(listener->usb);
    if(!listener->usb->configured) {
        processInputQueue(listener->serial);
    }
}
