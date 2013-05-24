#include "emqueue.h"
#include "interface/pipeline.h"
#include "util/log.h"
#include "util/bytebuffer.h"
#include "lights.h"

#define DROPPED_MESSAGE_LOGGING_THRESHOLD 100

using openxc::interface::uart::processUartSendQueue;
using openxc::interface::uart::uartConnected;
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

void openxc::interface::sendMessage(Pipeline* pipeline, uint8_t* message, int messageSize) {
    if(pipeline->usb->configured && !conditionalEnqueue(
                &pipeline->usb->sendQueue, message, messageSize)) {
        droppedMessage(USB);
    }

    if(uartConnected(pipeline->uart) && !conditionalEnqueue(
                &pipeline->uart->sendQueue, message, messageSize)) {
        droppedMessage(UART);
    }

    if(pipeline->network != NULL && !conditionalEnqueue(
                &pipeline->network->sendQueue, message, messageSize)) {
        droppedMessage(NETWORK);
    }
}

void openxc::interface::processPipelineQueues(Pipeline* pipeline) {
    // Must always process USB, because this function usually runs the MCU's USB
    // task that handles SETUP and enumeration.
    processUsbSendQueue(pipeline->usb);
    if(uartConnected(pipeline->uart)) {
        processUartSendQueue(pipeline->uart);
    }

    if(pipeline->network != NULL) {
       processNetworkSendQueue(pipeline->network);
    }
}
