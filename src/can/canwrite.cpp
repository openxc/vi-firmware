#include <canutil/write.h>
#include "can/canwrite.h"
#include "util/log.h"

namespace can = openxc::can;

using openxc::util::log::debugNoNewline;

QUEUE_DEFINE(CanMessage);

void checkWritePermission(CanSignal* signal, bool* send) {
    if(!signal->writable) {
        *send = false;
    }
}

void encodeSignal(CanSignal* signal, float value, uint8_t data[]) {
    bitfield_encode_float(value, signal->bitPosition, signal->bitSize,
            signal->factor, signal->offset, data);
}

void openxc::can::write::booleanWriter(CanSignal* signal,
        CanSignal* signals, int signalCount, bool value, uint8_t data[], bool* send) {
    return encodeSignal(signal, value, data);
}

void openxc::can::write::booleanWriter(CanSignal* signal,
        CanSignal* signals, int signalCount, cJSON* value, uint8_t data[], bool* send) {
    int intValue = 0;
    if(value->type == cJSON_False) {
        intValue = 0;
    } else if(value->type == cJSON_True) {
        intValue = 1;
    }
    return booleanWriter(signal, signals, signalCount, intValue, data, send);
}

void openxc::can::write::numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, double value, uint8_t data[], bool* send) {
    return encodeSignal(signal, value, data);
}

void openxc::can::write::numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, uint8_t data[], bool* send) {
    return numberWriter(signal, signals, signalCount, value->valuedouble, data, send);
}

void openxc::can::write::stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, const char* value, uint8_t data[], bool* send) {
    if(value == NULL) {
        debug("Can't write state of NULL -- not sending");
        *send = false;
    } else {
        const CanSignalState* signalState = lookupSignalState(value, signal,
                signals, signalCount);
        if(signalState != NULL) {
            encodeSignal(signal, signalState->value, data);
        } else {
            debug("Couldn't find a valid signal state for \"%s\"", value);
            *send = false;
        }
    }
}

void openxc::can::write::stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, uint8_t data[], bool* send) {
    if(value == NULL) {
        debug("Can't write state of NULL -- not sending");
        *send = false;
    } else {
        stateWriter(signal, signals, signalCount, value->valuestring, data,
                send);
    }
}

void openxc::can::write::enqueueMessage(CanBus* bus, CanMessage* message) {
    CanMessage outgoingMessage = {message->id};
    memcpy(outgoingMessage.data, message->data, CAN_MESSAGE_SIZE);
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
        void (*writer)(CanSignal*, CanSignal*, int, cJSON*, uint8_t[], bool*),
        CanSignal* signals, int signalCount) {
    return sendSignal(signal, value, writer, signals, signalCount, false);
}

bool openxc::can::write::sendSignal(CanSignal* signal, cJSON* value,
        void (*writer)(CanSignal*, CanSignal*, int, cJSON*, uint8_t[], bool*),
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

    uint8_t data[CAN_MESSAGE_SIZE] = {0};
    writer(signal, signals, signalCount, value, data, &send);
    if(force || send) {
        CanMessage message = {signal->message->id};
        memcpy(message.data, data, CAN_MESSAGE_SIZE);
        enqueueMessage(signal->message->bus, &message);
    } else {
        debug("Writing not allowed for signal with name %s",
                signal->genericName);
    }
    return send;
}

void openxc::can::write::processWriteQueue(CanBus* bus) {
    while(!QUEUE_EMPTY(CanMessage, &bus->sendQueue)) {
        CanMessage message = QUEUE_POP(CanMessage, &bus->sendQueue);
        debugNoNewline("Sending CAN message on bus 0x%03x: id = 0x%03x, data = 0x",
                bus->address, message.id);
        for(int i = 0; i < 8; i++) {
            debugNoNewline("%02x ", ((uint8_t*)&message.data)[i]);
        }
        debug("");
        if(bus->writeHandler == NULL) {
            debug("No function available for writing to CAN -- dropped");
        } else if(!bus->writeHandler(bus, message)) {
            debug("Unable to send CAN message with id = 0x%x", message.id);
        }
    }
}
