#include "canread.h"

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
    return value;
}

char* stateHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    CanSignalState* signalState = lookupSignalState(value, signal, signals,
            signalCount);
    if(signalState != NULL) {
        return signalState->name;
    }
    *send = false;
    return NULL;
}
