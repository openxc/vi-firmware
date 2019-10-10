#include <bitfield/8byte.h>
#include <canutil/write.h>
#include "can/canwrite.h"
#include "util/log.h"

namespace can = openxc::can;

using openxc::util::log::debug;

QUEUE_DEFINE(CanMessage);

void openxc::can::write::buildMessage(const CanSignal* signal, uint64_t value,
        uint8_t data[], size_t length) {
    bitfield_encode_float(value, signal->bitPosition, signal->bitSize,
            signal->factor, signal->offset, data, length);
}

uint64_t openxc::can::write::encodeBoolean(const CanSignal* signal, bool value,
        bool* send) {
    return encodeNumber(signal, float(value), send);
}

uint64_t openxc::can::write::encodeState(const CanSignal* signal, const char* state,
        bool* send) {
    uint64_t value = 0;
    if(state == NULL) {
        debug("Can't write state of NULL -- not sending");
        *send = false;
    } else {
        const CanSignalState* signalState = lookupSignalState(state, signal);
        if(signalState != NULL) {
            value = signalState->value;
        } else {
            debug("Couldn't find a valid signal state for \"%s\"", state);
            *send = false;
        }
    }
    return value;
}

uint64_t openxc::can::write::encodeNumber(const CanSignal* signal, float value,
        bool* send) {
    return float_to_fixed_point(value, signal->factor, signal->offset);
}

void openxc::can::write::enqueueMessage(CanBus* bus, CanMessage* message) {
    CanMessage outgoingMessage = {
        id: message->id,
        format: message->format
    };
    memcpy(outgoingMessage.data, message->data, CAN_MESSAGE_SIZE);
    outgoingMessage.length = (uint8_t)(message->length == 0 ?
            CAN_MESSAGE_SIZE : message->length);
    QUEUE_PUSH(CanMessage, &bus->sendQueue, outgoingMessage);
}

uint64_t openxc::can::write::encodeDynamicField(const CanSignal* signal,
        openxc_DynamicField* field, bool* send) {
    uint64_t value = 0;
    switch(field->type) {
        case openxc_DynamicField_Type_STRING:
            value = encodeState(signal, field->string_value, send);
            break;
        case openxc_DynamicField_Type_NUM:
            value = encodeNumber(signal, field->numeric_value, send);
            break;
        case openxc_DynamicField_Type_BOOL:
            value = encodeBoolean(signal, field->boolean_value, send);
            break;
        default:
            debug("Dynamic field didn't have a value, can't encode");
            *send = false;
            break;
    }
    return value;
}

bool openxc::can::write::encodeAndSendSignal(CanSignal* signal,
        openxc_DynamicField* value, bool force) {
    return encodeAndSendSignal(signal, value, signal->encoder, force);
}

bool openxc::can::write::encodeAndSendSignal(CanSignal* signal,
        openxc_DynamicField* value, SignalEncoder encoder, bool force) {
    bool send = true;
    uint64_t encodedValue = 0;
    if(encoder == NULL) {
        encodedValue = encodeDynamicField(signal, value, &send);
    } else {
        encodedValue = encoder(signal, value, &send);
    }

    if(force || send) {
        send = sendEncodedSignal(signal, encodedValue, force);
    }
    return send;
}

bool openxc::can::write::encodeAndSendNumericSignal(CanSignal* signal, float value, bool force) {
    //openxc_DynamicField field = {0};
    openxc_DynamicField field = openxc_DynamicField();	// Zero fill
    //field.has_type = true;
    field.type = openxc_DynamicField_Type_NUM;
    //field.has_numeric_value = true;
    field.numeric_value = value;
    return encodeAndSendSignal(signal, &field, force);
}

bool openxc::can::write::encodeAndSendStateSignal(CanSignal* signal, const char* value,
        bool force) {
    //openxc_DynamicField field = {0};
    openxc_DynamicField field = openxc_DynamicField();		// Zero fill
    //field.has_type = true;
    field.type = openxc_DynamicField_Type_STRING;
    //field.has_string_value = true;
    strcpy(field.string_value, value);
    return encodeAndSendSignal(signal, &field, force);
}

bool openxc::can::write::encodeAndSendBooleanSignal(CanSignal* signal, bool value, bool force) {
    //openxc_DynamicField field = {0};
    openxc_DynamicField field = openxc_DynamicField();		// Zero fill
    //field.has_type = true;
    field.type = openxc_DynamicField_Type_BOOL;
    //field.has_boolean_value = true;
    field.boolean_value = value;
    return encodeAndSendSignal(signal, &field, force);
}

bool openxc::can::write::sendEncodedSignal(CanSignal* signal, uint64_t value, bool force) {
    bool send = signal->writable;

    uint8_t data[CAN_MESSAGE_SIZE] = {0};
    buildMessage(signal, value, data, sizeof(data));
    if(force || send) {
        send = true;
        CanMessage message = {
            id: signal->message->id,
            format: signal->message->format
        };
        memcpy(message.data, data, CAN_MESSAGE_SIZE);
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
    debug("Sending CAN message on bus 0x%03x: id = 0x%03x, data = 0x%02x%02x%02x%02x%02x%02x%02x%02x",
        bus->address, message->id,
        ((uint8_t*)&message->data)[0],
        ((uint8_t*)&message->data)[1],
        ((uint8_t*)&message->data)[2],
        ((uint8_t*)&message->data)[3],
        ((uint8_t*)&message->data)[4],
        ((uint8_t*)&message->data)[5],
        ((uint8_t*)&message->data)[6],
        ((uint8_t*)&message->data)[7]
    );

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
