#include <bitfield/8byte.h>
#include <canutil/write.h>
#include "can/canwrite.h"
#include "util/log.h"

namespace can = openxc::can;

using openxc::util::log::debug;

QUEUE_DEFINE(CanMessage);

uint64_t openxc::can::write::buildMessage(CanSignal* signal, int value) {
    uint64_t result = 0;
    if(!eightbyte_set_bitfield(value, signal->bitPosition, signal->bitSize, &result)) {
        debug("Unable to encode value %d into a bitfield", value);
    }
    return result;
}

uint64_t openxc::can::write::encodeBoolean(const CanSignal* signal, bool value, bool* send) {
    return encodeNumber(signal, float(value), send);
}

uint64_t openxc::can::write::encodeState(const CanSignal* signal, const char* state, bool* send) {
    uint64_t result = 0;
    if(state == NULL) {
        debug("Can't write state of NULL -- not sending");
        *send = false;
    } else {
        const CanSignalState* signalState = lookupSignalState(state, signal);
        if(signalState != NULL) {
            result = signalState->value;
        } else {
            debug("Couldn't find a valid signal state for \"%s\"", state);
            *send = false;
        }
    }
    return encodeNumber(signal, result, send);
}

uint64_t openxc::can::write::encodeNumber(const CanSignal* signal, float value, bool* send) {
    return float_to_fixed_point(value, signal->factor, signal->offset);
}

void openxc::can::write::enqueueMessage(CanBus* bus, CanMessage* message) {
    CanMessage outgoingMessage = {
            id: message->id,
            format: message->format,
            data: __builtin_bswap64(message->data),
            length: (uint8_t)(message->length == 0 ? 8 : message->length)
    };
    QUEUE_PUSH(CanMessage, &bus->sendQueue, outgoingMessage);
}

uint64_t openxc::can::write::encodeDynamicField(const CanSignal* signal,
        openxc_DynamicField* field, bool* send) {
    float encodedValue = 0;
    switch(field->type) {
        case openxc_DynamicField_Type_STRING:
            encodedValue = encodeState(signal, field->string_value, send);
            break;
        case openxc_DynamicField_Type_NUM:
            encodedValue = encodeNumber(signal, field->numeric_value, send);
            break;
        case openxc_DynamicField_Type_BOOL:
            encodedValue = encodeBoolean(signal, field->numeric_value, send);
            break;
        default:
            debug("Dynamic field didn't have a value, can't encode");
            *send = false;
            break;
    }
    return encodedValue;
}

bool openxc::can::write::encodeAndSendSignal(CanSignal* signal,
        openxc_DynamicField* value, bool force) {
    return encodeAndSendSignal(signal, value, signal->writeHandler, force);
}

bool openxc::can::write::encodeAndSendSignal(CanSignal* signal,
        openxc_DynamicField* value, SignalEncoder writer, bool force) {
    bool send = true;
    uint64_t encodedValue = 0;
    if(writer == NULL) {
        encodedValue = encodeDynamicField(signal, value, &send);
    } else {
        // TODO do we need to pass the full payload into the handler?
        encodedValue = writer(signal, value, &send);
    }

    if(force || send) {
        send = sendEncodedSignal(signal, encodedValue, force);
    }
    return send;
}

bool openxc::can::write::encodeAndSendNumericSignal(CanSignal* signal, float value, bool force) {
    openxc_DynamicField field;
    field.has_type = true;
    field.type = openxc_DynamicField_Type_NUM;
    field.numeric_value = value;
    return encodeAndSendSignal(signal, &field, force);
}

bool openxc::can::write::encodeAndSendStateSignal(CanSignal* signal, const char* value,
        bool force) {
    openxc_DynamicField field;
    field.has_type = true;
    field.type = openxc_DynamicField_Type_STRING;
    strcpy(field.string_value, value);
    return encodeAndSendSignal(signal, &field, force);
}

bool openxc::can::write::encodeAndSendBooleanSignal(CanSignal* signal, bool value, bool force) {
    openxc_DynamicField field;
    field.has_type = true;
    field.type = openxc_DynamicField_Type_BOOL;
    field.boolean_value = value;
    return encodeAndSendSignal(signal, &field, force);
}

// value is already encoded
bool openxc::can::write::sendEncodedSignal(CanSignal* signal, uint64_t value, bool force) {
    bool send = signal->writable;

    uint64_t data = buildMessage(signal, value);
    if(force || send) {
        send = true;
        CanMessage message = {
            id: signal->message->id,
            format: signal->message->format,
            data: data
        };
        enqueueMessage(signal->message->bus, &message);
    } else {
        debug("Writing not allowed for signal with name %s",
                signal->genericName);
    }
    return send;
}

void openxc::can::write::flushOutgoingCanMessageQueue(CanBus* bus) {
    while(!QUEUE_EMPTY(CanMessage, &bus->sendQueue)) {
        const CanMessage message = QUEUE_POP(CanMessage, &bus->sendQueue);
        sendCanMessage(bus, &message);
    }
}

bool openxc::can::write::sendCanMessage(const CanBus* bus, const CanMessage* message) {
    debug("Sending CAN message on bus 0x%03x: id = 0x%03x, data = 0x%02llx",
            bus->address, message->id, __builtin_bswap64(message->data));
    bool status = true;
    if(bus->writeHandler == NULL) {
        debug("No function available for writing to CAN -- dropped");
        status = false;
    } else if(!bus->writeHandler(bus, message)) {
        debug("Unable to send CAN message with id = 0x%x", message->id);
        status = false;
    }
    return status;
}
