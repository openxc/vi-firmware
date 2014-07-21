#include <stdlib.h>
#include <canutil/read.h>
#include <pb_encode.h>
#include "can/canread.h"
#include "config.h"
#include "util/log.h"
#include "util/timer.h"

using openxc::util::log::debug;
using openxc::pipeline::MessageClass;
using openxc::pipeline::Pipeline;
using openxc::config::getConfiguration;
using openxc::pipeline::publish;

namespace pipeline = openxc::pipeline;
namespace time = openxc::util::time;

float openxc::can::read::parseSignalBitfield(CanSignal* signal,
        const CanMessage* message) {
    return bitfield_parse_float(message->data, CAN_MESSAGE_SIZE,
            signal->bitPosition, signal->bitSize, signal->factor,
            signal->offset);
}

openxc_DynamicField openxc::can::read::noopDecoder(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return payload::wrapNumber(value);
}

openxc_DynamicField openxc::can::read::booleanDecoder(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return payload::wrapBoolean(value == 0.0 ? false : true);
}

openxc_DynamicField openxc::can::read::ignoreDecoder(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    *send = false;
    openxc_DynamicField decodedValue = {0};
    return decodedValue;
}

openxc_DynamicField openxc::can::read::stateDecoder(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    openxc_DynamicField decodedValue = {0};
    decodedValue.has_type = true;
    decodedValue.type = openxc_DynamicField_Type_STRING;
    decodedValue.has_string_value = true;

    const CanSignalState* signalState = lookupSignalState(value, signal);
    if(signalState != NULL) {
        strcpy(decodedValue.string_value, signalState->name);
    } else {
        *send = false;
    }
    return decodedValue;
}

static void buildBaseTranslated(openxc_VehicleMessage* message,
        const char* name) {
    message->has_type = true;
    message->type = openxc_VehicleMessage_Type_TRANSLATED;
    message->has_translated_message = true;
    message->translated_message = {0};
    message->translated_message.has_name = true;
    strcpy(message->translated_message.name, name);
    message->translated_message.has_type = true;
}

void openxc::can::read::publishVehicleMessage(const char* name,
        openxc_DynamicField* value, openxc_DynamicField* event,
        openxc::pipeline::Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    buildBaseTranslated(&message, name);
    if(event == NULL) {
        switch(value->type) {
            case openxc_DynamicField_Type_STRING:
                message.translated_message.type =
                        openxc_TranslatedMessage_Type_STRING;
                break;
            case openxc_DynamicField_Type_NUM:
                message.translated_message.type =
                        openxc_TranslatedMessage_Type_NUM;
                break;
            case openxc_DynamicField_Type_BOOL:
                message.translated_message.type =
                        openxc_TranslatedMessage_Type_BOOL;
                break;
        }
    } else {
        switch(event->type) {
            case openxc_DynamicField_Type_STRING:
                message.translated_message.type =
                        openxc_TranslatedMessage_Type_EVENTED_STRING;
                break;
            case openxc_DynamicField_Type_NUM:
                message.translated_message.type =
                        openxc_TranslatedMessage_Type_EVENTED_NUM;
                break;
            case openxc_DynamicField_Type_BOOL:
                message.translated_message.type =
                        openxc_TranslatedMessage_Type_EVENTED_BOOL;
                break;
        }
    }

    if(value != NULL) {
        message.translated_message.has_value = true;
        message.translated_message.value = *value;
    }

    if(event != NULL) {
        message.translated_message.has_event = true;
        message.translated_message.event = *event;
    }

    pipeline::publish(&message, pipeline);
}

void openxc::can::read::publishVehicleMessage(const char* name,
        openxc_DynamicField* value, openxc::pipeline::Pipeline* pipeline) {
    publishVehicleMessage(name, value, NULL, pipeline);
}

void openxc::can::read::publishNumericalMessage(const char* name, float value,
        openxc::pipeline::Pipeline* pipeline) {
    openxc_DynamicField decodedValue = payload::wrapNumber(value);
    publishVehicleMessage(name, &decodedValue, pipeline);
}

void openxc::can::read::publishStringMessage(const char* name,
        const char* value, openxc::pipeline::Pipeline* pipeline) {
    openxc_DynamicField decodedValue = payload::wrapString(value);
    publishVehicleMessage(name, &decodedValue, pipeline);
}

void openxc::can::read::publishBooleanMessage(const char* name, bool value,
        openxc::pipeline::Pipeline* pipeline) {
    openxc_DynamicField decodedValue = payload::wrapBoolean(value);
    publishVehicleMessage(name, &decodedValue, pipeline);
}

void openxc::can::read::passthroughMessage(CanBus* bus, CanMessage* message,
        CanMessageDefinition* messages, int messageCount, Pipeline* pipeline) {
    bool send = true;
    CanMessageDefinition* messageDefinition = lookupMessageDefinition(bus,
            message->id, message->format, messages, messageCount);
    if(messageDefinition == NULL) {
        if(registerMessageDefinition(bus, message->id, message->format,
                    messages, messageCount)) {
            debug("Added new message definition for message %d on bus %d",
                    message->id, bus->address);
        // else you couldn't add it to the list for some reason, but don't
        // spam the log about it.
        }
    } else if(time::conditionalTick(&messageDefinition->frequencyClock) ||
            (memcmp(message->data, messageDefinition->lastValue,
                    CAN_MESSAGE_SIZE) &&
                 messageDefinition->forceSendChanged)) {
        send = true;
    } else {
        send = false;
    }

    size_t adjustedSize = message->length == 0 ?
            CAN_MESSAGE_SIZE : message->length;
    if(send) {
        openxc_VehicleMessage vehicleMessage = {0};
        vehicleMessage.has_type = true;
        vehicleMessage.type = openxc_VehicleMessage_Type_RAW;
        vehicleMessage.has_raw_message = true;
        vehicleMessage.raw_message = {0};
        vehicleMessage.raw_message.has_message_id = true;
        vehicleMessage.raw_message.message_id = message->id;
        vehicleMessage.raw_message.has_bus = true;
        vehicleMessage.raw_message.bus = bus->address;
        vehicleMessage.raw_message.has_data = true;
        vehicleMessage.raw_message.data.size = adjustedSize;
        memcpy(vehicleMessage.raw_message.data.bytes, message->data,
                adjustedSize);

        pipeline::publish(&vehicleMessage, pipeline);
    }

    if(messageDefinition != NULL) {
        memcpy(messageDefinition->lastValue, message->data, adjustedSize);
    }
}

void openxc::can::read::translateSignal(CanSignal* signal,
        const CanMessage* message,
        CanSignal* signals, int signalCount,
        openxc::pipeline::Pipeline* pipeline) {
    if(signal == NULL || message == NULL) {
        return;
    }

    float value = parseSignalBitfield(signal, message);

    bool send = true;
    // Must call the decoders every time, regardless of if we are going to
    // decide to send the signal or not.
    openxc_DynamicField decodedValue = openxc::can::read::decodeSignal(signal,
            value, signals, signalCount, &send);
    if(send && shouldSend(signal, value)) {
        if(send) {
            openxc::can::read::publishVehicleMessage(signal->genericName, &decodedValue, pipeline);
        }
    }
    signal->received = true;
    signal->lastValue = value;
}

bool openxc::can::read::shouldSend(CanSignal* signal, float value) {
    bool send = true;
    if(time::conditionalTick(&signal->frequencyClock) ||
            (value != signal->lastValue && signal->forceSendChanged)) {
        if(signal->received && !signal->sendSame
                && value == signal->lastValue) {
            send = false;
        }
    } else {
        send = false;
    }
    return send;
}

openxc_DynamicField openxc::can::read::decodeSignal(CanSignal* signal,
        float value, CanSignal* signals, int signalCount, bool* send) {
    SignalDecoder decoder = signal->decoder == NULL ?
            noopDecoder : signal->decoder;
    openxc_DynamicField decodedValue = decoder(signal, signals,
            signalCount, NULL, value, send);
    return decodedValue;
}

openxc_DynamicField openxc::can::read::decodeSignal(CanSignal* signal,
        const CanMessage* message, CanSignal* signals, int signalCount,
        bool* send) {
    float value = parseSignalBitfield(signal, message);
    return decodeSignal(signal, value, signals, signalCount, send);
}
