#include "emqueue.h"
#include "pipeline.h"
#include "util/log.h"
#include "util/timer.h"
#include "util/statistics.h"
#include "util/bytebuffer.h"
#include "config.h"
#include "lights.h"
#define PIPELINE_ENDPOINT_COUNT 5
#define PIPELINE_STATS_LOG_FREQUENCY_S 15
#define QUEUE_FLUSH_MAX_TRIES 100
#include "platform_profile.h"
#ifdef RTC_SUPPORT
	#include "platform/pic32/rtc.h"
#endif

namespace uart = openxc::interface::uart;
namespace usb = openxc::interface::usb;
namespace ble =  openxc::interface::ble;
namespace fs   =  openxc::interface::fs;
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
using openxc::interface::InterfaceDescriptor;
using openxc::interface::InterfaceType;
using openxc::config::LoggingOutputInterface;
using openxc::util::time::uptimeMs;

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

void sendToEndpoint(openxc::interface::InterfaceType endpointType,
        QUEUE_TYPE(uint8_t)* sendQueue, QUEUE_TYPE(uint8_t)* receiveQueue,
        uint8_t* message, int messageSize) {
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
            if(config::getConfiguration()->loggingOutput !=
                        LoggingOutputInterface::BOTH &&
                    config::getConfiguration()->loggingOutput !=
                        LoggingOutputInterface::USB) {
                return;
            }
        } else {
            sendQueue = &pipeline->usb->endpoints[IN_ENDPOINT_INDEX].queue;
        }

        conditionalFlush(pipeline, sendQueue, message, messageSize);
        sendToEndpoint(pipeline->usb->descriptor.type, sendQueue,
                &pipeline->usb->endpoints[OUT_ENDPOINT_INDEX].queue,
                message, messageSize);
    }
}

void sendToUart(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass) {
    if(uart::connected(pipeline->uart) && messageClass != MessageClass::LOG) {
		//if(uart::connected(pipeline->uart)) {
        QUEUE_TYPE(uint8_t)* sendQueue = &pipeline->uart->sendQueue;
        conditionalFlush(pipeline, sendQueue, message, messageSize);
        sendToEndpoint(pipeline->uart->descriptor.type, sendQueue,
                &pipeline->uart->receiveQueue, message, messageSize);
    }
}

#ifdef TELIT_HE910_SUPPORT
void sendToTelit(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass) {
    if(openxc::telitHE910::connected(pipeline->telit) && messageClass != MessageClass::LOG) {
        QUEUE_TYPE(uint8_t)* sendQueue = &pipeline->telit->sendQueue;
        conditionalFlush(pipeline, sendQueue, message, messageSize);
        sendToEndpoint(pipeline->telit->descriptor.type, sendQueue, &pipeline->telit->receiveQueue, message,
                messageSize);
    }
    // removed UART logging from the telit
}
#endif

#ifdef BLE_SUPPORT
void sendToBle(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass) {
        
    if(ble::connected(pipeline->ble) && messageClass != MessageClass::LOG) { //TODO add a characteristic for sending debug notification messages
        QUEUE_TYPE(uint8_t)* sendQueue = (QUEUE_TYPE(uint8_t)* )&pipeline->ble->sendQueue;
        conditionalFlush(pipeline,sendQueue, message, messageSize);
        sendToEndpoint(pipeline->ble->descriptor.type, sendQueue,(QUEUE_TYPE(uint8_t)* )&pipeline->ble->receiveQueue, message,
                messageSize);
    }

}
#endif
#ifdef FS_SUPPORT
void sendToFS(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass) {
    if(fs::connected(pipeline->fs) && messageClass != MessageClass::LOG
                    && messageClass != MessageClass::COMMAND_RESPONSE
    ) { 
        QUEUE_TYPE(uint8_t)* sendQueue = (QUEUE_TYPE(uint8_t)* )&pipeline->fs->sendQueue;
        conditionalFlush(pipeline,sendQueue, message, messageSize);
        openxc::interface::InterfaceType endpointType = pipeline->fs->descriptor.type;
        if(!conditionalEnqueue(sendQueue, message, messageSize)) {
            ++droppedMessages[endpointType];
        } else {
        ++sentMessages[endpointType];
        dataSent[endpointType] += messageSize;
        }
        sendQueueLength[endpointType] = QUEUE_LENGTH(uint8_t, sendQueue);
    }
}
#endif


void sendToNetwork(Pipeline* pipeline, uint8_t* message, int messageSize,
        MessageClass messageClass) {
    if(pipeline->network != NULL && messageClass != MessageClass::LOG) {
        QUEUE_TYPE(uint8_t)* sendQueue = &pipeline->network->sendQueue;
        conditionalFlush(pipeline, sendQueue, message, messageSize);
        sendToEndpoint(pipeline->network->descriptor.type, sendQueue,
                &pipeline->network->receiveQueue, message, messageSize);
    }
}

void openxc::pipeline::publish(openxc_VehicleMessage* message,
        Pipeline* pipeline) {
    uint8_t payload[MAX_OUTGOING_PAYLOAD_SIZE] = {0};
    #ifdef RTC_SUPPORT
    message->timestamp = syst.tm;
    #elif defined TELIT_HE910_SUPPORT
    message->timestamp = uptimeMs();
    #endif
    
    size_t length = payload::serialize(message, payload, sizeof(payload),
            config::getConfiguration()->payloadFormat);
    MessageClass messageClass;
    bool matched = false;
    switch(message->type) {
        case openxc_VehicleMessage_Type_SIMPLE:
            messageClass = MessageClass::SIMPLE;
            matched = true;
            break;
        case openxc_VehicleMessage_Type_CAN:
            messageClass = MessageClass::CAN;
            matched = true;
            break;
        case openxc_VehicleMessage_Type_DIAGNOSTIC:
            messageClass = MessageClass::DIAGNOSTIC;
            matched = true;
            break;
        case openxc_VehicleMessage_Type_COMMAND_RESPONSE:
            messageClass = MessageClass::COMMAND_RESPONSE;
            matched = true;
            break;
        case openxc_VehicleMessage_Type_CONTROL_COMMAND:
        default:
            break;
    }
    if(matched) {
        sendMessage(pipeline, payload, length, messageClass);
    } else {
        debug("Trying to serialize unrecognized type: %d", message->type);
    }
}

void openxc::pipeline::sendMessage(Pipeline* pipeline, uint8_t* message,
        int messageSize, MessageClass messageClass) {
    sendToUsb(pipeline, message, messageSize, messageClass);
    #ifdef TELIT_HE910_SUPPORT
    sendToTelit(pipeline, message, messageSize, messageClass);
    #elif defined BLE_SUPPORT
    sendToBle(pipeline, message, messageSize, messageClass);
    #else
    //#ifndef FS_SUPPORT //UART shared with RTC, disable
    sendToUart(pipeline, message, messageSize, messageClass);
    //#endif
    #endif
    #ifdef FS_SUPPORT
    sendToFS(pipeline, message, messageSize, messageClass);
    #endif
    
    sendToNetwork(pipeline, message, messageSize, messageClass);

    if((config::getConfiguration()->loggingOutput == LoggingOutputInterface::BOTH ||
        config::getConfiguration()->loggingOutput == LoggingOutputInterface::UART)
            && messageClass == MessageClass::LOG) {
        openxc::util::log::debugUart((const char*)message);
        openxc::util::log::debugUart("\r\n");
    }
}

void openxc::pipeline::process(Pipeline* pipeline) {
    // Must always process USB, because this function usually runs the MCU's USB
    // task that handles SETUP and enumeration.
    usb::processSendQueue(pipeline->usb);
    #ifdef TELIT_HE910_SUPPORT
    if(telitHE910::connected(pipeline->telit)) {
        telitHE910::processSendQueue(pipeline->telit);
    }
    #endif
    #ifdef BLE_SUPPORT
    if(ble::connected(pipeline->ble)){
        ble::processSendQueue(pipeline->ble);
    }
    #endif
    #ifdef FS_SUPPORT
    if(fs::connected(pipeline->fs)){
        fs::processSendQueue(pipeline->fs);
    }
    #endif
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
            statistics::update(&sentMessageStats[i], sentMessages[i]);
            statistics::update(&droppedMessageStats[i], droppedMessages[i]);
            statistics::update(&totalMessageStats[i],
                    sentMessages[i] + droppedMessages[i]);
            statistics::update(&dataSentStats[i], dataSent[i]);
            statistics::update(&dataSentStats[i], dataSent[i]);

            statistics::update(&sendQueueStats[i], sendQueueLength[i]);
            statistics::update(&receiveQueueStats[i], receiveQueueLength[i]);

            if(totalMessageStats[i].total > 0) {
                InterfaceDescriptor descriptor;
                descriptor.type = (InterfaceType) i;
                debug("%s avg queue fill percents, Rx: %f, Tx: %f",
                        descriptorToString(&descriptor),
                        statistics::exponentialMovingAverage(&receiveQueueStats[i])
                            / QUEUE_MAX_LENGTH(uint8_t) * 100,
                        statistics::exponentialMovingAverage(&sendQueueStats[i])
                            / QUEUE_MAX_LENGTH(uint8_t) * 100);
                debug("%s msgs sent: %d, dropped: %d (avg %f percent)",
                        descriptorToString(&descriptor),
                        sentMessageStats[i].total,
                        droppedMessageStats[i].total,
                        statistics::exponentialMovingAverage(&droppedMessageStats[i]) /
                            statistics::exponentialMovingAverage(&totalMessageStats[i]) * 100);
                debug("%s avg throughput: %fKB / s, %d msgs / s",
                        descriptorToString(&descriptor),
                        statistics::exponentialMovingAverage(&dataSentStats[i])
                            / 1024.0 / PIPELINE_STATS_LOG_FREQUENCY_S,
                        (int)(statistics::exponentialMovingAverage(&sentMessageStats[i])
                            / PIPELINE_STATS_LOG_FREQUENCY_S));
            }
            lastTimeLogged = time::systemTimeMs();
        }
    }
}
