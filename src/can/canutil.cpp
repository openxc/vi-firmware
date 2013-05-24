#include "can/canutil.h"
#include "can/canwrite.h"
#include "util/timer.h"
#include "util/log.h"

using openxc::util::time::systemTimeMs;
using openxc::util::log::debugNoNewline;

const int openxc::can::CAN_ACTIVE_TIMEOUT_S = 30;

void openxc::can::initializeCanCommon(CanBus* bus) {
    debugNoNewline("Initializing CAN node 0x%2x...", bus->address);
    QUEUE_INIT(CanMessage, &bus->receiveQueue);
    QUEUE_INIT(CanMessage, &bus->sendQueue);
    bus->writeHandler = openxc::can::write::sendCanMessage;
    bus->lastMessageReceived = 0;
}

bool openxc::can::canBusActive(CanBus* bus) {
    return bus->lastMessageReceived != 0 &&
        systemTimeMs() - bus->lastMessageReceived < CAN_ACTIVE_TIMEOUT_S * 1000;
}

int lookup(void* key,
        bool (*comparator)(void* key, int index, void* candidates),
        void* candidates, int candidateCount) {
    for(int i = 0; i < candidateCount; i++) {
        if(comparator(key, i, candidates)) {
            return i;
        }
    }
    return -1;
}

bool signalStateNameComparator(void* name, int index, void* states) {
    return !strcmp((const char*)name, ((CanSignalState*)states)[index].name);
}

CanSignalState* openxc::can::lookupSignalState(const char* name, CanSignal* signal,
        CanSignal* signals, int signalCount) {
    int index = lookup((void*)name, signalStateNameComparator,
            (void*)signal->states, signal->stateCount);
    if(index != -1) {
        return &signal->states[index];
    } else {
        return NULL;
    }
}

bool signalStateValueComparator(void* value, int index, void* states) {
    return (*(int*)value) == ((CanSignalState*)states)[index].value;
}

CanSignalState* openxc::can::lookupSignalState(int value, CanSignal* signal,
        CanSignal* signals, int signalCount) {
    int index = lookup((void*)&value, signalStateValueComparator,
            (void*)signal->states, signal->stateCount);
    if(index != -1) {
        return &signal->states[index];
    } else {
        return NULL;
    }
}

bool signalComparator(void* name, int index, void* signals) {
    return !strcmp((const char*)name, ((CanSignal*)signals)[index].genericName);
}

bool writableSignalComparator(void* name, int index, void* signals) {
    return signalComparator(name, index, signals) &&
            ((CanSignal*)signals)[index].writable;
}

CanSignal* openxc::can::lookupSignal(const char* name, CanSignal* signals, int signalCount,
        bool writable) {
    bool (*comparator)(void* key, int index, void* candidates) = signalComparator;
    if(writable) {
        comparator = writableSignalComparator;
    }
    int index = lookup((void*)name, comparator, (void*)signals, signalCount);
    if(index != -1) {
        return &signals[index];
    } else {
        return NULL;
    }
}

CanSignal* openxc::can::lookupSignal(const char* name, CanSignal* signals, int signalCount) {
    return lookupSignal(name, signals, signalCount, false);
}

bool commandComparator(void* name, int index, void* commands) {
    return !strcmp((const char*)name, ((CanCommand*)commands)[index].genericName);
}

CanCommand* openxc::can::lookupCommand(const char* name, CanCommand* commands, int commandCount) {
    int index = lookup((void*)name, commandComparator, (void*)commands,
            commandCount);
    if(index != -1) {
        return &commands[index];
    } else {
        return NULL;
    }
}
