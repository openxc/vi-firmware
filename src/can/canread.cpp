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
using openxc::pipeline::sendVehicleMessage;

namespace time = openxc::util::time;

float openxc::can::read::preTranslate(CanSignal* signal, uint64_t data,
        bool* send) {
    float value = eightbyte_parse_float(data, signal->bitPosition,
            signal->bitSize, signal->factor, signal->offset);

    if(time::conditionalTick(&signal->frequencyClock) ||
            (value != signal->lastValue && signal->forceSendChanged)) {
        if(send && (!signal->received || signal->sendSame ||
                    value != signal->lastValue)) {
            signal->received = true;
        } else {
            *send = false;
        }
    } else {
        *send = false;
    }
    return value;
}

void openxc::can::read::postTranslate(CanSignal* signal, float value) {
    signal->lastValue = value;
}

float openxc::can::read::passthroughHandler(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return value;
}

bool openxc::can::read::booleanHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    return value == 0.0 ? false : true;
}

float openxc::can::read::ignoreHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    *send = false;
    return value;
}

const char* openxc::can::read::stateHandler(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    const CanSignalState* signalState = lookupSignalState(value, signal);
    if(signalState != NULL) {
        return signalState->name;
    }
    *send = false;
    return NULL;
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

void openxc::can::read::sendNumericalMessage(const char* name, float value,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    buildBaseTranslated(&message, name);
    message.translated_message.type = openxc_TranslatedMessage_Type_NUM;
    message.translated_message.has_value = true;
    message.translated_message.value.has_numeric_value = true;
    message.translated_message.value.numeric_value = value;

    sendVehicleMessage(&message, pipeline);
}

void openxc::can::read::sendBooleanMessage(const char* name, bool value,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    buildBaseTranslated(&message, name);
    message.translated_message.type = openxc_TranslatedMessage_Type_BOOL;
    message.translated_message.has_value = true;
    message.translated_message.value.has_boolean_value = true;
    message.translated_message.value.boolean_value = value;

    sendVehicleMessage(&message, pipeline);
}

void openxc::can::read::sendStringMessage(const char* name, const char* value,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    buildBaseTranslated(&message, name);
    message.translated_message.type = openxc_TranslatedMessage_Type_STRING;
    message.translated_message.has_value = true;
    message.translated_message.value.has_string_value = true;
    strcpy(message.translated_message.value.string_value, value);

    sendVehicleMessage(&message, pipeline);
}

void openxc::can::read::sendEventedFloatMessage(const char* name,
        const char* value, float event,
        Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    buildBaseTranslated(&message, name);
    message.translated_message.type = openxc_TranslatedMessage_Type_EVENTED_NUM;
    message.translated_message.has_value = true;
    message.translated_message.value.has_string_value = true;
    strcpy(message.translated_message.value.string_value, value);
    message.translated_message.has_event = true;
    message.translated_message.event.has_numeric_value = true;
    message.translated_message.event.numeric_value = event;

    sendVehicleMessage(&message, pipeline);
}

void openxc::can::read::sendEventedBooleanMessage(const char* name,
        const char* value, bool event, Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    buildBaseTranslated(&message, name);
    message.translated_message.type =
            openxc_TranslatedMessage_Type_EVENTED_BOOL;
    message.translated_message.has_value = true;
    message.translated_message.value.has_string_value = true;
    strcpy(message.translated_message.value.string_value, value);
    message.translated_message.has_event = true;
    message.translated_message.event.has_boolean_value = true;
    message.translated_message.event.boolean_value = event;

    sendVehicleMessage(&message, pipeline);
}

void openxc::can::read::sendEventedStringMessage(const char* name,
        const char* value, const char* event, Pipeline* pipeline) {
    openxc_VehicleMessage message = {0};
    buildBaseTranslated(&message, name);
    message.translated_message.type =
            openxc_TranslatedMessage_Type_EVENTED_STRING;
    message.translated_message.has_value = true;
    message.translated_message.value.has_string_value = true;
    strcpy(message.translated_message.value.string_value, value);
    message.translated_message.has_event = true;
    message.translated_message.event.has_string_value = true;
    strcpy(message.translated_message.event.string_value, event);

    sendVehicleMessage(&message, pipeline);
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
            (message->data != messageDefinition->lastValue &&
                 messageDefinition->forceSendChanged)) {
        send = true;
    } else {
        send = false;
    }

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
        vehicleMessage.raw_message.data.size = message->length;

        union {
            uint64_t whole;
            uint8_t bytes[8];
        } combined;
        combined.whole = message->data;
        memcpy(vehicleMessage.raw_message.data.bytes, combined.bytes,
                message->length);

        sendVehicleMessage(&vehicleMessage, pipeline);
    }

    if(messageDefinition != NULL) {
        messageDefinition->lastValue = message->data;
    }
}

void openxc::can::read::translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data, NumericalHandler handler, CanSignal* signals,
        int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    float processedValue = handler(signal, signals, signalCount, pipeline,
            value, &send);
    if(send) {
        sendNumericalMessage(signal->genericName, processedValue, pipeline);
    }
    postTranslate(signal, value);
}

void openxc::can::read::translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data, StringHandler handler, CanSignal* signals,
        int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    const char* stringValue = handler(signal, signals, signalCount, pipeline,
            value, &send);
    if(stringValue != NULL && send) {
        sendStringMessage(signal->genericName, stringValue, pipeline);
    }
    postTranslate(signal, value);
}

void openxc::can::read::translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data, BooleanHandler handler, CanSignal* signals,
        int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    bool booleanValue = handler(signal, signals, signalCount, pipeline, value,
            &send);
    if(send) {
        sendBooleanMessage(signal->genericName, booleanValue, pipeline);
    }
    postTranslate(signal, value);
}

void openxc::can::read::translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data, CanSignal* signals, int signalCount) {
    translateSignal(pipeline, signal, data, passthroughHandler, signals,
            signalCount);
}
