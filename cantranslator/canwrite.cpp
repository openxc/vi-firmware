#include "canwrite.h"

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
    unsigned long rawValue = (value - signal->offset) / signal->factor;
    if(rawValue > 0) {
        // round up to avoid losing precision when we cast to an int
        rawValue += 0.5;
    }
    setBitField(&data, rawValue, signal->bitPosition, signal->bitSize);
    return data;
}
