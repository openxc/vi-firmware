#include "listener.h"
#include "log.h"
#include "buffers.h"

bool conditionalEnqueue(QUEUE_TYPE(uint8_t)* queue, uint8_t* message,
        int messageSize) {
    if(queue_available(queue) < messageSize + 2) {
        return false;
    }

    for(int i = 0; i < messageSize; i++) {
        QUEUE_PUSH(uint8_t, queue, (uint8_t)message[i]);
    }
    QUEUE_PUSH(uint8_t, queue, (uint8_t)'\r');
    QUEUE_PUSH(uint8_t, queue, (uint8_t)'\n');
    return true;
}

void sendMessage(Listener* listener, uint8_t* message, int messageSize) {
    if(!conditionalEnqueue(&listener->usb->sendQueue, message, messageSize)) {
        debug("USB send queue full, dropping CAN message: %s\r\n", message);
    }

    if(!conditionalEnqueue(&listener->serial->sendQueue, message, messageSize)) {
        debug("UART send queue full, dropping CAN message: %s\r\n", message);
    }
}

void processListenerQueues(Listener* listener) {
    // Must always process USB, because this function usually runs the MCU's USB
    // task that handles SETUP and enumeration.
    processInputQueue(listener->usb);
    processInputQueue(listener->serial);
}
