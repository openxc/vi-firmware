#include "canutil.h"

CanSignalState* lookupSignalState(CanSignal* signal, CanSignal* signals,
        int signalCount, char* name) {
    for(int i = 0; i < signal->stateCount; i++) {
        if(!strcmp(signal->states[i].name, name)) {
            return &signal->states[i];
        }
    }
    return NULL;
}

CanSignalState* lookupSignalState(CanSignal* signal, CanSignal* signals,
        int signalCount, int value) {
    for(int i = 0; i < signal->stateCount; i++) {
        if(signal->states[i].value == value) {
            return &signal->states[i];
        }
    }
    return NULL;
}

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
