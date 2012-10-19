#include "queue.h"
#include "listener.h"
#include "log.h"
#include "buffers.h"

bool conditionalEnqueue(QUEUE_TYPE(uint8_t)* queue, uint8_t* message,
        int messageSize) {
    if(QUEUE_AVAILABLE(uint8_t, queue) < messageSize + 2) {
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
    if(listener->usb->configured && !conditionalEnqueue(
                &listener->usb->sendQueue, message, messageSize)) {
        debug("USB send queue full, dropping CAN message: %s\r\n", message);
    }

#ifndef NO_UART
    if(!conditionalEnqueue(&listener->serial->sendQueue, message, messageSize)) {
        debug("UART send queue full, dropping CAN message: %s\r\n", message);
    }
#endif // NO_UART

#ifndef NO_ETHERNET
    if(!conditionalEnqueue(&listener->ethernet->sendQueue, message, messageSize)) {
           debug("Ethernet send queue full, dropping CAN message: %s\r\n", message);
       }
#endif // NO_ETHERNET
}

void processListenerQueues(Listener* listener) {
    // Must always process USB, because this function usually runs the MCU's USB
    // task that handles SETUP and enumeration.
    processUsbSendQueue(listener->usb);
#ifndef NO_UART
    processSerialSendQueue(listener->serial);
#endif // NO_UART
}
