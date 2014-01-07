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

uint64_t encodeSignal(CanSignal* signal, float value) {
    return eightbyte_encode_float(value, signal->bitPosition, signal->bitSize,
            signal->factor, signal->offset);
}

uint64_t openxc::can::write::booleanWriter(CanSignal* signal,
        CanSignal* signals, int signalCount, bool value, bool* send) {
    return encodeSignal(signal, value);
}

uint64_t openxc::can::write::booleanWriter(CanSignal* signal,
        CanSignal* signals, int signalCount, cJSON* value, bool* send) {
    int intValue = 0;
    if(value->type == cJSON_False) {
        intValue = 0;
    } else if(value->type == cJSON_True) {
        intValue = 1;
    }
    return booleanWriter(signal, signals, signalCount, intValue, send);
}

uint64_t openxc::can::write::numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, double value, bool* send) {
    return encodeSignal(signal, value);
}

uint64_t openxc::can::write::numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send) {
    return numberWriter(signal, signals, signalCount, value->valuedouble, send);
}

uint64_t openxc::can::write::stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, const char* value, bool* send) {
    uint64_t result = 0;
    if(value == NULL) {
        debug("Can't write state of NULL -- not sending");
        *send = false;
    } else {
        const CanSignalState* signalState = lookupSignalState(value, signal,
                signals, signalCount);
        if(signalState != NULL) {
            result = encodeSignal(signal, signalState->value);
        } else {
            debug("Couldn't find a valid signal state for \"%s\"", value);
            *send = false;
        }
    }
    return result;
}

uint64_t openxc::can::write::stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send) {
    uint64_t result = 0;
    if(value == NULL) {
        debug("Can't write state of NULL -- not sending");
        *send = false;
    } else {
        result = stateWriter(signal, signals, signalCount, value->valuestring,
                send);
    }
    return result;
}

void openxc::can::write::enqueueMessage(CanBus* bus, CanMessage* message) {
    CanMessage outgoingMessage = {message->id,
            __builtin_bswap64(message->data)};
    QUEUE_PUSH(CanMessage, &bus->sendQueue, outgoingMessage);
}

bool openxc::can::write::sendSignal(CanSignal* signal, cJSON* value,
        CanSignal* signals, int signalCount) {
    return sendSignal(signal, value, signals, signalCount, false);
}

bool openxc::can::write::sendSignal(CanSignal* signal, cJSON* value,
        CanSignal* signals, int signalCount, bool force) {
    return sendSignal(signal, value, signal->writeHandler, signals,
            signalCount, force);
}

bool openxc::can::write::sendSignal(CanSignal* signal, cJSON* value,
        uint64_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*),
        CanSignal* signals, int signalCount) {
    return sendSignal(signal, value, writer, signals, signalCount, false);
}

bool openxc::can::write::sendSignal(CanSignal* signal, cJSON* value,
        uint64_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*),
        CanSignal* signals, int signalCount, bool force) {
    if(writer == NULL) {
        if(signal->stateCount > 0) {
            writer = stateWriter;
        } else {
            writer = numberWriter;
        }
    }
    bool send = true;
    checkWritePermission(signal, &send);

    uint64_t data = writer(signal, signals, signalCount, value, &send);
    if(force || send) {
        CanMessage message = {signal->message->id, data};
        enqueueMessage(signal->message->bus, &message);
    } else {
        debug("Writing not allowed for signal with name %s",
                signal->genericName);
    }
    return send;
}

void openxc::can::write::processWriteQueue(CanBus* bus) {
    while(!QUEUE_EMPTY(CanMessage, &bus->sendQueue)) {
        const CanMessage message = QUEUE_POP(CanMessage, &bus->sendQueue);
        sendCanMessage(bus, &message);
    }
}

bool openxc::can::write::sendCanMessage(const CanBus* bus, const CanMessage* message) {
    debug("Sending CAN message on bus 0x%03x: id = 0x%03x, data = 0x%02llx",
            bus->address, message->id, message->data);
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
