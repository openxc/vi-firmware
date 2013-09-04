#include "can/canutil.h"
#include "can/canwrite.h"
#include "util/timer.h"
#include "util/log.h"

namespace time = openxc::util::time;

using openxc::util::log::debugNoNewline;

const int openxc::can::CAN_ACTIVE_TIMEOUT_S = 30;

void openxc::can::initializeCommon(CanBus* bus) {
    debugNoNewline("Initializing CAN node %d...", bus->address);
    QUEUE_INIT(CanMessage, &bus->receiveQueue);
    QUEUE_INIT(CanMessage, &bus->sendQueue);
    bus->writeHandler = openxc::can::write::sendMessage;
    bus->lastMessageReceived = 0;
}

bool openxc::can::busActive(CanBus* bus) {
    return bus->lastMessageReceived != 0 &&
        time::systemTimeMs() - bus->lastMessageReceived < CAN_ACTIVE_TIMEOUT_S * 1000;
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

void openxc::can::logBusStatistics(CanBus* buses, const int busCount) {
#ifdef __LOG_STATS__
    static unsigned long lastTimeLogged;
    if(time::systemTimeMs() - lastTimeLogged > BUS_STATS_LOG_FREQUENCY_S * 1000) {
        unsigned int totalMessagesReceived = 0;
        unsigned int totalMessagesDropped = 0;
        float totalDataKB = 0;
        for(int i = 0; i < busCount; i++) {
            CanBus* bus = buses[i];
            float busTotalDataKB = bus->messagesReceived *
                    CAN_MESSAGE_TOTAL_BIT_SIZE / 8192;
            debug("CAN messages received on bus %d: %d",
                    bus->address, bus->messagesReceived + bus->messagesDropped);
            debug("CAN messages processed on bus %d: %d",
                    bus->address, bus->messagesReceived);
            debug("CAN messages dropped on bus %d: %d",
                    bus->address, bus->messagesDropped);
            debug("Dropped message ratio: %f%%",
                    bus->messagesDropped / (float)(bus->messagesDropped + bus->messagesReceived));
            debug("Data received on bus %d: %f KB", bus->address,
                    busTotalDataKB);
            debug("Aggregate throughput on bus %d: %f KB / s", bus->address,
                    busTotalDataKB / (time::uptimeMs() / 1000.0));
            totalMessagesReceived += bus->messagesReceived;
            totalMessagesDropped += bus->messagesDropped;
            totalDataKB += busTotalDataKB;
        }

        debug("Total CAN messages dropped since startup on all buses: %d",
                totalMessagesDropped);
        debug("Aggregate message drop rate across all buses since startup: %f msgs / s",
                totalMessagesDropped / (time::uptimeMs() / 1000.0));
        debug("Dropped message ratio on all buses since startup: %f%%",
                totalMessagesDropped / (float)(totalMessagesDropped + totalMessagesReceived));
        debug("Total CAN messages received since startup on all buses: %d",
                totalMessagesReceived);
        debug("Aggregate message rate across all buses since startup: %f msgs / s",
                totalMessagesReceived / (time::uptimeMs() / 1000.0));
        debug("Aggregate throughput across all buses since startup: %f KB / s",
                totalDataKB / (time::uptimeMs() / 1000.0));

        openxc::pipeline::logStatistics(&pipeline);

        lastTimeLogged = time::systemTimeMs();
    }
#endif // __LOG_STATS__
}

