#include "canwrite.h"

void checkWritePermission(CanSignal* signal, bool* send) {
    if(!signal->writable) {
        *send = false;
    }
}

uint64_t booleanWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send) {
    return encodeCanSignal(signal, value->valueint);
}

uint64_t numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send) {
    checkWritePermission(signal, send);
    return encodeCanSignal(signal, value->valuedouble);
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
    unsigned long rawValue = (value - signal->offset) / signal->factor + 0.5;
    uint64_t data = 0;
    setBitField(&data, rawValue, signal->bitPosition, signal->bitSize);
    return data;
}
