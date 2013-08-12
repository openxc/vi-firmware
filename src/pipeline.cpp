#include "emqueue.h"
#include "pipeline.h"
#include "util/log.h"
#include "util/timer.h"
#include "util/bytebuffer.h"
#include "lights.h"

#define DROPPED_MESSAGE_LOGGING_THRESHOLD 100

namespace uart = openxc::interface::uart;
namespace usb = openxc::interface::usb;
namespace network = openxc::interface::network;
namespace time = openxc::util::time;

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
int sentMessages[3];

void openxc::pipeline::sendMessage(Pipeline* pipeline, uint8_t* message, int messageSize) {
    if(pipeline->usb->configured) {
        if(!conditionalEnqueue(&pipeline->usb->sendQueue, message,
                messageSize)) {
            ++droppedMessages[USB];
        } else {
            ++sentMessages[USB];
        }
    }

    if(uart::connected(pipeline->uart)) {
        if(!conditionalEnqueue(&pipeline->uart->sendQueue, message,
                messageSize)) {
            ++droppedMessages[UART];
        } else {
            ++sentMessages[UART];
        }
    }

    if(pipeline->network != NULL) {
        if(!conditionalEnqueue(
                &pipeline->network->sendQueue, message, messageSize)) {
            ++droppedMessages[NETWORK];
        } else {
            ++sentMessages[NETWORK];
        }
    }
}

void openxc::pipeline::process(Pipeline* pipeline) {
    // Must always process USB, because this function usually runs the MCU's USB
    // task that handles SETUP and enumeration.
    usb::processSendQueue(pipeline->usb);
    if(uart::connected(pipeline->uart)) {
        uart::processSendQueue(pipeline->uart);
    }

    if(pipeline->network != NULL) {
       network::processSendQueue(pipeline->network);
    }
}

void openxc::pipeline::logStatistics(Pipeline* pipeline) {
    debug("USB messages sent: %d", sentMessages[USB]);
    debug("USB messages dropped: %d", droppedMessages[USB]);
    float droppedRatio = droppedMessages[USB] / (float)(droppedMessages[USB] +
            sentMessages[USB]);
    debug("USB message drop ratio: %f", droppedRatio);
    debug("Aggregate USB sent message rate since startup: %f msgs / s",
            sentMessages[USB] / (time::uptimeMs() / 1000.0));
}
