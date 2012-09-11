#include "canwrite.h"
#include "log.h"

QUEUE_DEFINE(CanMessage);

void checkWritePermission(CanSignal* signal, bool* send) {
    if(!signal->writable) {
        *send = false;
    }
}

uint64_t booleanWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send) {
    return booleanWriter(signal, signals, signalCount, value, send, 0);
}

uint64_t booleanWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send, uint64_t data) {
    checkWritePermission(signal, send);
    return encodeCanSignal(signal, value->valueint, data);
}

uint64_t numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send) {
    return numberWriter(signal, signals, signalCount, value, send, 0);
}

uint64_t numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send, uint64_t data) {
    checkWritePermission(signal, send);
    return encodeCanSignal(signal, value->valuedouble, data);
}

uint64_t stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send) {
    CanSignalState* signalState = lookupSignalState(value->valuestring, signal,
            signals, signalCount);
    if(signalState != NULL) {
        checkWritePermission(signal, send);
        return encodeCanSignal(signal, signalState->value);
    }
    *send = false;
    return 0;
}

uint64_t encodeCanSignal(CanSignal* signal, float value) {
    return encodeCanSignal(signal, value, 0);
}

uint64_t encodeCanSignal(CanSignal* signal, float value, uint64_t data) {
    float rawValue = (value - signal->offset) / signal->factor;
    if(rawValue > 0) {
        // round up to avoid losing precision when we cast to an int
        rawValue += 0.5;
    }
    setBitField(&data, rawValue, signal->bitPosition, signal->bitSize);
    return data;
}

bool sendCanSignal(CanSignal* signal, uint64_t data, bool* send) {
    if(send) {
        CanMessage message = {signal->messageId, &data};
        QUEUE_PUSH(CanMessage, &signal->bus->sendQueue, message);
        return true;
    } else {
        debug("Not sending requested message %x", signal->messageId);
    }
}

bool sendCanSignal(CanSignal* signal, cJSON* value, CanSignal* signals,
        int signalCount) {
    uint64_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*)
        = signal->writeHandler;
    return sendCanSignal(signal, value, writer, signals, signalCount);
}

bool sendCanSignal(CanSignal* signal, cJSON* value,
        uint64_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*),
        CanSignal* signals, int signalCount) {
    bool send = true;
    if(writer == NULL) {
        if(signal->stateCount > 0) {
            writer = stateWriter;
        } else {
            writer = numberWriter;
        }
    }

    uint64_t data = writer(signal, signals, signalCount, value, &send);
    return sendCanSignal(signal, data, &send);
}
