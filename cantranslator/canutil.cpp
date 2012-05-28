#include "canutil.h"

uint32_t encodeCanSignal(CanSignal* signal, float value) {
    unsigned long rawValue = (value - signal->offset) / signal->factor;
    uint32_t data = 0;
    setBitField(&data, rawValue, signal->bitPosition, signal->bitSize);
    return data;
}

float decodeCanSignal(CanSignal* signal, uint8_t* data) {
    unsigned long rawValue = getBitField(data, signal->bitPosition,
            signal->bitSize);
    return rawValue * signal->factor + signal->offset;
}

float passthroughHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return value;
}

bool booleanHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return value == 0.0 ? false : true;
}

float ignoreHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    *send = false;
    return 0.0;
}

char* stateHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    for(int i = 0; i < signal->stateCount; i++) {
        if(signal->states[i].value == value) {
            return signal->states[i].name;
        }
    }
    *send = false;
}

uint32_t passthroughWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, uint32_t value, bool* send) {
    return encodeCanSignal(signal, value);
}

uint32_t numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    return encodeCanSignal(signal, value);
}

uint32_t stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, char* value, bool* send);

uint32_t booleanWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, bool value, bool* send);

CanSignal* lookupSignal(char* name, CanSignal* signals, int signalCount) {
    for(int i = 0; i < signalCount; i++) {
        CanSignal* signal = &signals[i];
        if(!strcmp(name, signal->genericName)) {
            return signal;
        }
    }
    printf("Couldn't find a signal with the genericName \"%s\" "
            "-- probably about to segfault\n", name);
    return NULL;
}
