#include "emqueue.h"
#include "pipeline.h"
#include "util/log.h"
#include "util/timer.h"
#include "util/statistics.h"
#include "util/bytebuffer.h"
#include "lights.h"

#define PIPELINE_ENDPOINT_COUNT 3
#define PIPELINE_STATS_LOG_FREQUENCY_S 15
#define QUEUE_FLUSH_MAX_TRIES 5

namespace uart = openxc::interface::uart;
namespace usb = openxc::interface::usb;
namespace network = openxc::interface::network;
namespace time = openxc::util::time;
namespace statistics = openxc::util::statistics;

using openxc::util::bytebuffer::conditionalEnqueue;
using openxc::util::bytebuffer::messageFits;
using openxc::util::statistics::DeltaStatistic;

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
unsigned int sendQueueLength[PIPELINE_ENDPOINT_COUNT];
unsigned int receiveQueueLength[PIPELINE_ENDPOINT_COUNT];

void openxc::pipeline::sendMessage(Pipeline* pipeline, uint8_t* message,
        int messageSize) {
    if(pipeline->usb->configured) {
        int timeout = QUEUE_FLUSH_MAX_TRIES;
        QUEUE_TYPE(uint8_t)* sendQueue =
                &pipeline->usb->endpoints[IN_ENDPOINT_NUMBER - 1].sendQueue;
        while(timeout > 0 && !messageFits(sendQueue, message, messageSize)) {
            process(pipeline);
            --timeout;
        }

        if(!conditionalEnqueue(sendQueue, message,
                messageSize)) {
            ++droppedMessages[USB];
        } else {
            ++sentMessages[USB];
            dataSent[USB] += messageSize;
        }
        sendQueueLength[USB] = QUEUE_LENGTH(uint8_t, sendQueue);

        // TODO This may not belong here after USB refactoring
        receiveQueueLength[USB] = QUEUE_LENGTH(uint8_t,
                &pipeline->usb->endpoints[IN_ENDPOINT_NUMBER - 1].receiveQueue);
    }

    if(uart::connected(pipeline->uart)) {
        int timeout = QUEUE_FLUSH_MAX_TRIES;
        while(timeout > 0 && !messageFits(&pipeline->uart->sendQueue, message,
                    messageSize)) {
            process(pipeline);
            --timeout;
        }

        if(!conditionalEnqueue(&pipeline->uart->sendQueue, message,
                messageSize)) {
            ++droppedMessages[UART];
        } else {
            ++sentMessages[UART];
            dataSent[UART] += messageSize;
        }
        sendQueueLength[UART] = QUEUE_LENGTH(uint8_t,
                &pipeline->uart->sendQueue);
        receiveQueueLength[UART] = QUEUE_LENGTH(uint8_t,
                &pipeline->uart->receiveQueue);
    }

    if(pipeline->network != NULL) {
        int timeout = QUEUE_FLUSH_MAX_TRIES;
        while(timeout > 0 && !messageFits(&pipeline->network->sendQueue,
                    message, messageSize)) {
            process(pipeline);
            --timeout;
        }

        if(!conditionalEnqueue(
                &pipeline->network->sendQueue, message, messageSize)) {
            ++droppedMessages[NETWORK];
        } else {
            ++sentMessages[NETWORK];
            dataSent[NETWORK] += messageSize;
        }
        sendQueueLength[NETWORK] = QUEUE_LENGTH(uint8_t,
                &pipeline->network->sendQueue);
        receiveQueueLength[NETWORK] = QUEUE_LENGTH(uint8_t,
                &pipeline->network->receiveQueue);
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
    static DeltaStatistic droppedMessageStats[PIPELINE_ENDPOINT_COUNT];
    static DeltaStatistic sentMessageStats[PIPELINE_ENDPOINT_COUNT];
    static DeltaStatistic totalMessageStats[PIPELINE_ENDPOINT_COUNT];
    static DeltaStatistic dataSentStats[PIPELINE_ENDPOINT_COUNT];
    static DeltaStatistic sendQueueStats[PIPELINE_ENDPOINT_COUNT];
    static DeltaStatistic receiveQueueStats[PIPELINE_ENDPOINT_COUNT];
    static bool initializedStats = false;
    if(!initializedStats) {
        for(int i = 0; i < PIPELINE_ENDPOINT_COUNT; i++) {
            statistics::initialize(&droppedMessageStats[i]);
            statistics::initialize(&sentMessageStats[i]);
            statistics::initialize(&totalMessageStats[i]);
            statistics::initialize(&dataSentStats[i]);
            statistics::initialize(&sendQueueStats[i]);
            statistics::initialize(&receiveQueueStats[i]);
        }
        initializedStats = true;
    }

    if(time::systemTimeMs() - lastTimeLogged >
            PIPELINE_STATS_LOG_FREQUENCY_S * 1000) {
        for(int i = 0; i < PIPELINE_ENDPOINT_COUNT; i++) {
            const char* interfaceName = messageTypeNames[i];
            statistics::update(&sentMessageStats[i], sentMessages[i]);
            statistics::update(&droppedMessageStats[i], droppedMessages[i]);
            statistics::update(&totalMessageStats[i],
                    sentMessages[i] + droppedMessages[i]);
            statistics::update(&dataSentStats[i], dataSent[i]);
            statistics::update(&dataSentStats[i], dataSent[i]);

            statistics::update(&sendQueueStats[i], sendQueueLength[i]);
            statistics::update(&receiveQueueStats[i], receiveQueueLength[i]);

            if(totalMessageStats[i].total > 0) {
                debug("%s avg queue fill percents, Rx: %f, Tx: %f",
                            interfaceName,
                        statistics::exponentialMovingAverage(&receiveQueueStats[i])
                            / QUEUE_MAX_LENGTH(uint8_t) * 100,
                        statistics::exponentialMovingAverage(&sendQueueStats[i])
                            / QUEUE_MAX_LENGTH(uint8_t) * 100);
                debug("%s msgs sent: %d, dropped: %d (avg %f percent)",
                        interfaceName,
                        sentMessageStats[i].total,
                        droppedMessageStats[i].total,
                        statistics::exponentialMovingAverage(&droppedMessageStats[i]) /
                            statistics::exponentialMovingAverage(&totalMessageStats[i]) * 100);
                debug("%s avg throughput: %fKB / s, %d msgs / s",
                        interfaceName,
                        statistics::exponentialMovingAverage(&dataSentStats[i])
                            / 1024.0 / PIPELINE_STATS_LOG_FREQUENCY_S,
                        (int)(statistics::exponentialMovingAverage(&sentMessageStats[i])
                            / PIPELINE_STATS_LOG_FREQUENCY_S));
            }
            lastTimeLogged = time::systemTimeMs();
        }
    }
#endif // __LOG_STATS__
}
