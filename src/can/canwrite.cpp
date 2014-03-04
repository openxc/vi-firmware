#include <canutil/write.h>
#include "can/canwrite.h"
#include "util/log.h"

namespace can = openxc::can;

using openxc::util::log::debug;

QUEUE_DEFINE(CanMessage);

void checkWritePermission(CanSignal* signal, bool* send) {
    if(!signal->writable) {
        *send = false;
    }
}

uint64_t openxc::can::write::encodeSignal(CanSignal* signal, float value) {
    return eightbyte_encode_float(value, signal->bitPosition, signal->bitSize,
            signal->factor, signal->offset);
}

float openxc::can::write::numberEncoder(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    return value;
}

float openxc::can::write::numberEncoder(CanSignal* signal, CanSignal* signals,
        int signalCount, openxc_DynamicField* value, bool* send) {
    return numberEncoder(signal, signals, signalCount, value->numeric_value,
            send);
}

float openxc::can::write::stateEncoder(CanSignal* signal, CanSignal* signals,
        int signalCount, const char* value, bool* send) {
    float result = 0;
    if(value == NULL) {
        debug("Can't write state of NULL -- not sending");
        *send = false;
    } else {
        const CanSignalState* signalState = lookupSignalState(value, signal,
                signals, signalCount);
        if(signalState != NULL) {
            result = signalState->value;
        } else {
            debug("Couldn't find a valid signal state for \"%s\"", value);
            *send = false;
        }
    }
    return result;
}

float openxc::can::write::stateEncoder(CanSignal* signal, CanSignal* signals,
        int signalCount, openxc_DynamicField* value, bool* send) {
    float result = 0;
    if(value == NULL) {
        debug("Can't write state of NULL -- not sending");
        *send = false;
    } else {
        result = stateEncoder(signal, signals, signalCount, value->string_value,
                send);
    }
    return result;
}

void openxc::can::write::enqueueMessage(CanBus* bus, CanMessage* message) {
    CanMessage outgoingMessage = {
            id: message->id,
            data: __builtin_bswap64(message->data),
            length: (uint8_t)(message->length == 0 ? 8 : message->length)
    };
    QUEUE_PUSH(CanMessage, &bus->sendQueue, outgoingMessage);
}

bool openxc::can::write::sendSignal(CanSignal* signal, openxc_DynamicField* value,
        CanSignal* signals, int signalCount) {
    return sendSignal(signal, value, signals, signalCount, false);
}

bool openxc::can::write::sendSignal(CanSignal* signal,
        openxc_DynamicField* value, CanSignal* signals, int signalCount,
        bool force) {
    return sendSignal(signal, value, signal->writeHandler, signals,
            signalCount, force);
}

bool openxc::can::write::sendSignal(CanSignal* signal, openxc_DynamicField* value,
        SignalEncoder writer, CanSignal* signals, int signalCount) {
    return sendSignal(signal, value, writer, signals, signalCount, false);
}

bool openxc::can::write::sendSignal(CanSignal* signal,
        openxc_DynamicField* value, SignalEncoder writer, CanSignal* signals,
        int signalCount, bool force) {
    if(writer == NULL) {
        if(signal->stateCount > 0) {
            writer = stateEncoder;
        } else {
            writer = numberEncoder;
        }
    }

    bool send = true;
    float rawValue = writer(signal, signals, signalCount, value, &send);
    if(force || send) {
        return sendSignal(signal, rawValue, signals, signalCount, force);
    }
    return send;
}

bool openxc::can::write::sendSignal(CanSignal* signal, float value, CanSignal* signals,
                int signalCount, bool force) {
    bool send = true;
    checkWritePermission(signal, &send);

    uint64_t data = encodeSignal(signal, value);
    if(force || send) {
        CanMessage message = {signal->message->id, data};
        enqueueMessage(signal->message->bus, &message);
    } else {
        debug("Writing not allowed for signal with name %s",
                signal->genericName);
    }
    return send;
}

bool openxc::can::write::sendSignal(CanSignal* signal, float value, CanSignal* signals,
                int signalCount) {
    return sendSignal(signal, value, signals, signalCount);
}

void openxc::can::write::processWriteQueue(CanBus* bus) {
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
