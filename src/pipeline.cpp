#include "emqueue.h"
#include "pipeline.h"
#include "util/log.h"
#include "util/timer.h"
#include "util/bytebuffer.h"
#include "lights.h"

#define PIPELINE_ENDPOINT_COUNT 3
#define PIPELINE_STATS_LOG_FREQUENCY_S 5

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

unsigned int droppedMessages[PIPELINE_ENDPOINT_COUNT];
unsigned int sentMessages[PIPELINE_ENDPOINT_COUNT];
unsigned int dataSent[PIPELINE_ENDPOINT_COUNT];

void openxc::pipeline::sendMessage(Pipeline* pipeline, uint8_t* message,
        int messageSize) {
    if(pipeline->usb->configured) {
        if(!conditionalEnqueue(&pipeline->usb->sendQueue, message,
                messageSize)) {
            ++droppedMessages[USB];
        } else {
            ++sentMessages[USB];
            dataSent[USB] += messageSize;
        }
    }

    if(uart::connected(pipeline->uart)) {
        if(!conditionalEnqueue(&pipeline->uart->sendQueue, message,
                messageSize)) {
            ++droppedMessages[UART];
        } else {
            ++sentMessages[UART];
            dataSent[UART] += messageSize;
        }
    }

    if(pipeline->network != NULL) {
        if(!conditionalEnqueue(
                &pipeline->network->sendQueue, message, messageSize)) {
            ++droppedMessages[NETWORK];
        } else {
            ++sentMessages[NETWORK];
            dataSent[NETWORK] += messageSize;
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
#ifdef __LOG_STATS__
    static unsigned long lastTimeLogged;

    if(time::systemTimeMs() - lastTimeLogged > PIPELINE_STATS_LOG_FREQUENCY_S * 1000) {
        for(int i = 0; i < PIPELINE_ENDPOINT_COUNT; i++) {
            const char* interfaceName = messageTypeNames[i];
            debug("%s messages sent: %d", interfaceName, sentMessages[i]);
            debug("%s messages dropped: %d", interfaceName, droppedMessages[i]);
            float droppedRatio = 0;
            if(droppedMessages[i] > 0) {
                droppedRatio = droppedMessages[i] / (float)(droppedMessages[i] +
                        sentMessages[i]);
            }
            debug("%s message drop ratio: %f", interfaceName, droppedRatio);
            debug("Aggregate %s sent message rate since startup: %f msgs / s",
                    interfaceName, sentMessages[i] / (time::uptimeMs() / 1000.0));
            // TODO this isn't that accurate if the interface was dropping messages
            // when it wasn't connected - it would be more useful if it was "time
            // since connected"
            debug("Average %s data rate since startup: %fKB / s",
                    interfaceName, dataSent[i] / 1024 / (time::uptimeMs() / 1000.0));
            lastTimeLogged = time::systemTimeMs();
        }
    }
#endif // __LOG_STATS__
}
