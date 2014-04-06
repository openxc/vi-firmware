#include "emqueue.h"
#include "pipeline.h"
#include "util/log.h"
#include "util/timer.h"
#include "util/statistics.h"
#include "util/bytebuffer.h"
#include "config.h"
#include "lights.h"

#define PIPELINE_ENDPOINT_COUNT 3
#define PIPELINE_STATS_LOG_FREQUENCY_S 15
#define QUEUE_FLUSH_MAX_TRIES 5

namespace uart = openxc::interface::uart;
namespace usb = openxc::interface::usb;
namespace network = openxc::interface::network;
namespace time = openxc::util::time;
namespace statistics = openxc::util::statistics;
namespace config = openxc::config;

using openxc::util::bytebuffer::conditionalEnqueue;
using openxc::util::bytebuffer::messageFits;
using openxc::util::statistics::DeltaStatistic;
using openxc::util::log::debug;
using openxc::pipeline::Pipeline;
using openxc::pipeline::MessageClass;

typedef enum {
    USB = 0,
    UART = 1,
    NETWORK = 2
} EndpointType;

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

void conditionalFlush(Pipeline* pipeline,
        QUEUE_TYPE(uint8_t)* sendQueue, uint8_t* message, int messageSize) {
    int timeout = QUEUE_FLUSH_MAX_TRIES;
    while(timeout > 0 && !messageFits(sendQueue, message, messageSize)) {
        process(pipeline);
        --timeout;
    }
}

void sendToEndpoint(EndpointType endpointType, QUEUE_TYPE(uint8_t)* sendQueue,
        QUEUE_TYPE(uint8_t)* receiveQueue, uint8_t* message, int messageSize) {
    if(!conditionalEnqueue(sendQueue, message, messageSize)) {
        ++droppedMessages[endpointType];
    } else {
        ++sentMessages[endpointType];
        dataSent[endpointType] += messageSize;
    }
    sendQueueLength[endpointType] = QUEUE_LENGTH(uint8_t, sendQueue);
    // TODO This may not belong here after USB refactoring
    receiveQueueLength[endpointType] = QUEUE_LENGTH(uint8_t, receiveQueue);
}

void sendToUsb(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass) {
    if(pipeline->usb->configured) {
        QUEUE_TYPE(uint8_t)* sendQueue;
        if(messageClass == MessageClass::LOG) {
            sendQueue = &pipeline->usb->endpoints[LOG_ENDPOINT_INDEX].queue;
        } else {
            sendQueue = &pipeline->usb->endpoints[IN_ENDPOINT_INDEX].queue;
        }

        conditionalFlush(pipeline, sendQueue, message, messageSize);
        sendToEndpoint(USB, sendQueue,
                &pipeline->usb->endpoints[OUT_ENDPOINT_INDEX].queue,
                message, messageSize);
    }
}

void sendToUart(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass) {
    if(uart::connected(pipeline->uart) && messageClass != MessageClass::LOG) {
        QUEUE_TYPE(uint8_t)* sendQueue = &pipeline->uart->sendQueue;
        conditionalFlush(pipeline, sendQueue, message, messageSize);
        sendToEndpoint(UART, sendQueue, &pipeline->uart->receiveQueue, message,
                messageSize);
    }

    if(config::getConfiguration()->uartLogging &&
            messageClass == MessageClass::LOG) {
        openxc::util::log::debugUart((const char*)message);
        openxc::util::log::debugUart("\r\n");
    }
}

void sendToNetwork(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass) {
    if(pipeline->network != NULL && messageClass != MessageClass::LOG) {
        QUEUE_TYPE(uint8_t)* sendQueue = &pipeline->network->sendQueue;
        conditionalFlush(pipeline, sendQueue, message, messageSize);
        sendToEndpoint(NETWORK, sendQueue, &pipeline->network->receiveQueue,
                message, messageSize);
    }
}

void openxc::pipeline::publish(openxc_VehicleMessage* message,
        Pipeline* pipeline) {
    uint8_t payload[MAX_OUTGOING_PAYLOAD_SIZE] = {0};
    size_t length = payload::serialize(message, payload, sizeof(payload),
            config::getConfiguration()->payloadFormat);
    MessageClass messageClass;
    switch(message->type) {
        case openxc_VehicleMessage_Type_TRANSLATED:
            messageClass = MessageClass::TRANSLATED;
            break;
        case openxc_VehicleMessage_Type_RAW:
            messageClass = MessageClass::RAW;
            break;
        case openxc_VehicleMessage_Type_DIAGNOSTIC:
            messageClass = MessageClass::DIAGNOSTIC;
            break;
        case openxc_VehicleMessage_Type_COMMAND_RESPONSE:
            messageClass = MessageClass::COMMAND_RESPONSE;
            break;
        default:
            debug("Trying to serialize unrecognized type: %d", message->type);
            break;
    }
    sendMessage(pipeline, payload, length, messageClass);
}

void openxc::pipeline::sendMessage(Pipeline* pipeline, uint8_t* message,
        int messageSize, MessageClass messageClass) {
    sendToUsb(pipeline, message, messageSize, messageClass);
    sendToUart(pipeline, message, messageSize, messageClass);
    sendToNetwork(pipeline, message, messageSize, messageClass);
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
    if(!config::getConfiguration()->calculateMetrics) {
        return;
    }

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
}
