#include "emqueue.h"
#include "interface/pipeline.h"
#include "util/log.h"
#include "util/bytebuffer.h"
#include "lights.h"

#define DROPPED_MESSAGE_LOGGING_THRESHOLD 100

using openxc::interface::uart::processSerialSendQueue;
using openxc::util::bytebuffer::conditionalEnqueue;

typedef enum {
    USB = 0,
    UART = 1,
    NETWORK = 2
} MessageType;

const char messageTypeNames[][9] = {
    "USB",
    "UART",
    "Network",
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

void openxc::interface::sendMessage(Listener* listener, uint8_t* message, int messageSize) {
    if(listener->usb->configured && !conditionalEnqueue(
                &listener->usb->sendQueue, message, messageSize)) {
        droppedMessage(USB);
    }

    if(listener->serial != NULL && !conditionalEnqueue(
                &listener->serial->sendQueue, message, messageSize)) {
        droppedMessage(UART);
    }

    if(listener->network != NULL && !conditionalEnqueue(
                &listener->network->sendQueue, message, messageSize)) {
        droppedMessage(NETWORK);
    }
}

void openxc::interface::processListenerQueues(Listener* listener) {
    // Must always process USB, because this function usually runs the MCU's USB
    // task that handles SETUP and enumeration.
    processUsbSendQueue(listener->usb);
    if(listener->serial != NULL) {
        processSerialSendQueue(listener->serial);
    }

    if(listener->network != NULL) {
       processNetworkSendQueue(listener->network);
    }
}
