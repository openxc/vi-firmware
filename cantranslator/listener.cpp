#include "listener.h"

void sendMessage(Listener* listener, uint8_t* message, int messageSize) {
    sendMessage(listener->usb, message, messageSize);
    sendMessage(listener->serial, message, messageSize);
}

void processListenerQueues(Listener* listener) {
    processInputQueue(listener->usb);
    processInputQueue(listener->serial);
}
