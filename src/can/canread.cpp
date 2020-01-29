#include <stdlib.h>
#include <canutil/read.h>
#include <pb_encode.h>
#include "can/canread.h"
#include "config.h"
#include "util/log.h"
#include "util/timer.h"
#include <stdio.h>

using openxc::util::log::debug;
using openxc::pipeline::MessageClass;
using openxc::pipeline::Pipeline;
using openxc::config::getConfiguration;
using openxc::pipeline::publish;

namespace pipeline = openxc::pipeline;
namespace time = openxc::util::time;

float openxc::can::read::parseSignalBitfield(const CanSignal* signal,
        const CanMessage* message) {
    return bitfield_parse_float(message->data, CAN_MESSAGE_SIZE,
            signal->bitPosition, signal->bitSize, signal->factor,
            signal->offset);
}

openxc_DynamicField openxc::can::read::noopDecoder(const CanSignal* signal,
        const CanSignal* signals, SignalManager* signalManager, SignalManager* signalManagers,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    return payload::wrapNumber(value);
}

openxc_DynamicField openxc::can::read::booleanDecoder(const CanSignal* signal,
        const CanSignal* signals, SignalManager* signalManager, SignalManager* signalManagers, 
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    return payload::wrapBoolean(value == 0.0 ? false : true);
}

openxc_DynamicField openxc::can::read::ignoreDecoder(const CanSignal* signal,
        const CanSignal* signals, SignalManager* signalManager, SignalManager* signalManagers, 
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    *send = false;
    openxc_DynamicField decodedValue = openxc_DynamicField();		// Zero fill
    return decodedValue;
}

openxc_DynamicField openxc::can::read::stateDecoder(const CanSignal* signal,
        const CanSignal* signals,  SignalManager* signalManager,  SignalManager* signalManagers, 
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField decodedValue = openxc_DynamicField();		// Zero fill
    decodedValue.type = openxc_DynamicField_Type_STRING;

    const CanSignalState* signalState = lookupSignalState(value, signal);
    if(signalState != NULL) {
        strcpy(decodedValue.string_value, signalState->name);
    } else {
        *send = false;
    }
    return decodedValue;
}

static void buildBaseSimpleVehicleMessage(openxc_VehicleMessage* message,
        const char* name) {
    message->type = openxc_VehicleMessage_Type_SIMPLE;
    message->simple_message = openxc_SimpleMessage();		// Zero Fill
    strcpy(message->simple_message.name, name);
}

void openxc::can::read::publishVehicleMessage(const char* name,
        openxc_DynamicField* value, openxc_DynamicField* event,
        openxc::pipeline::Pipeline* pipeline) {
    openxc_VehicleMessage message = openxc_VehicleMessage();		// Zero fill
    buildBaseSimpleVehicleMessage(&message, name);

    if(value != NULL) {
        message.simple_message.value = *value;
    }

    if(event != NULL) {
        message.simple_message.event = *event;
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

void openxc::can::read::publishStringEventedMessage(const char* name,
        const char* value, const char* event, openxc::pipeline::Pipeline* pipeline) {
    openxc_DynamicField decodedValue = payload::wrapString(value);
	openxc_DynamicField decodedEvent = payload::wrapString(event);
    publishVehicleMessage(name, &decodedValue, &decodedEvent, pipeline);
}

void openxc::can::read::publishStringEventedBooleanMessage(const char* name,
        const char* value, bool event, openxc::pipeline::Pipeline* pipeline) {
    openxc_DynamicField decodedValue = payload::wrapString(value);
	openxc_DynamicField decodedEvent = payload::wrapBoolean(event);
    publishVehicleMessage(name, &decodedValue, &decodedEvent, pipeline);
}

void openxc::can::read::publishBooleanMessage(const char* name, bool value,
        openxc::pipeline::Pipeline* pipeline) {
    openxc_DynamicField decodedValue = payload::wrapBoolean(value);
    publishVehicleMessage(name, &decodedValue, pipeline);
}

void openxc::can::read::passthroughMessage(CanBus* bus, CanMessage* message,
        const CanMessageDefinition* messages, int messageCount, Pipeline* pipeline) {
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
        openxc_VehicleMessage vehicleMessage = openxc_VehicleMessage();		// Zero fill
        vehicleMessage.type = openxc_VehicleMessage_Type_CAN;
        vehicleMessage.can_message = {0};
        vehicleMessage.can_message.id = message->id;
        vehicleMessage.can_message.bus = bus->address;
        vehicleMessage.can_message.data.size = adjustedSize;
        memcpy(vehicleMessage.can_message.data.bytes, message->data,
                adjustedSize);

        pipeline::publish(&vehicleMessage, pipeline);
    }

    if(messageDefinition != NULL) {
        memcpy(messageDefinition->lastValue, message->data, adjustedSize);
    }
}

void openxc::can::read::translateSignal(const CanSignal* signal, CanMessage* message,
        const CanSignal* signals, SignalManager* signalManagers, int signalCount,
        openxc::pipeline::Pipeline* pipeline) {
    SignalManager* signalManager = lookupSignalManagerDetails(signal->genericName, signalManagers, signalCount);
    if(signal == NULL || message == NULL || signalManager == NULL) {
        return;
    }

    float value = parseSignalBitfield(signal, message);
    bool send = true;
    // Must call the decoders every time, regardless of if we are going to
    // decide to send the signal or not.
    openxc_DynamicField decodedValue = openxc::can::read::decodeSignal(signal, signals, 
        signalManager, signalManagers, signalCount, value, &send);

    if(send && shouldSend(signal, signalManager, value)) {
        openxc::can::read::publishVehicleMessage(signal->genericName, &decodedValue, pipeline);
    }

    signalManager->received = true;
    signalManager->lastValue = value;
}



bool openxc::can::read::shouldSend(const CanSignal* signal, SignalManager* signalManager, float value) {
    bool send = true;
    if(time::conditionalTick((time::FrequencyClock*) &signalManager->frequencyClock) ||
            (value != signalManager->lastValue && signal->forceSendChanged)) {
        if(signalManager->received && !signal->sendSame
                && value == signalManager->lastValue) {
            send = false;
        }
    } else {
        send = false;
    }
    return send;
}

openxc_DynamicField openxc::can::read::decodeSignal(const CanSignal* signal, const CanSignal* signals, 
    SignalManager* signalManager, SignalManager* signalManagers, int signalCount, 
    float value, bool* send) {
    SignalDecoder decoder = signal->decoder == NULL ?
            noopDecoder : signal->decoder;
    openxc_DynamicField decodedValue = decoder(signal, signals, signalManager, signalManagers,
            signalCount, &getConfiguration()->pipeline, value, send);
    return decodedValue;
}

openxc_DynamicField openxc::can::read::decodeSignal(const CanSignal* signal, const CanSignal* signals, 
        SignalManager* signalManager, SignalManager* signalManagers, int signalCount,
        const CanMessage* message, bool* send) {
    float value = parseSignalBitfield(signal, message);
    return decodeSignal(signal, signals, signalManager, signalManagers, signalCount, value, send);
}
