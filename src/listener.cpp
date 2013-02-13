#include "queue.h"
#include "listener.h"
#include "log.h"
#include "buffers.h"

#define DROPPED_MESSAGE_LOGGING_THRESHOLD 100

typedef enum {
    USB = 0,
    UART = 1,
    ETHERNET = 2
} MessageType;

const char messageTypeNames[][9] = {
    "USB",
    "UART",
    "Ethernet",
};

int droppedMessages[3];

void droppedMessage(MessageType type) {
    droppedMessages[type]++;
    if(droppedMessages[type] > DROPPED_MESSAGE_LOGGING_THRESHOLD) {
        droppedMessages[type] = 0;
        debug("%s send queue full, dropped another %d messages",
                messageTypeNames[type], DROPPED_MESSAGE_LOGGING_THRESHOLD);
    }
}

void sendMessage(Listener* listener, uint8_t* message, int messageSize) {
    if(listener->usb->configured && !conditionalEnqueue(
                &listener->usb->sendQueue, message, messageSize)) {
        droppedMessage(USB);
    }

    if(listener->serial != NULL && !conditionalEnqueue(
                &listener->serial->sendQueue, message, messageSize)) {
        droppedMessage(UART);
    }

    if(listener->ethernet != NULL && !conditionalEnqueue(
                &listener->ethernet->sendQueue, message, messageSize)) {
        droppedMessage(ETHERNET);
    }
}

void processListenerQueues(Listener* listener) {
    // Must always process USB, because this function usually runs the MCU's USB
    // task that handles SETUP and enumeration.
    processUsbSendQueue(listener->usb);
    if(listener->serial != NULL) {
        processSerialSendQueue(listener->serial);
    }

    if(listener->ethernet != NULL) {
       processEthernetSendQueue(listener->ethernet);
    }
}
