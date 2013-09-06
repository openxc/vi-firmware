#include "can/canutil.h"
#include "can/canwrite.h"
#include "util/timer.h"
#include "util/statistics.h"
#include "util/log.h"

#define BUS_STATS_LOG_FREQUENCY_S 5
#define CAN_MESSAGE_TOTAL_BIT_SIZE 128

namespace time = openxc::util::time;
namespace statistics = openxc::util::statistics;

using openxc::util::log::debugNoNewline;
using openxc::util::statistics::DeltaStatistic;

const int openxc::can::CAN_ACTIVE_TIMEOUT_S = 30;

void openxc::can::initializeCommon(CanBus* bus) {
    debugNoNewline("Initializing CAN node %d...", bus->address);
    QUEUE_INIT(CanMessage, &bus->receiveQueue);
    QUEUE_INIT(CanMessage, &bus->sendQueue);
    bus->writeHandler = openxc::can::write::sendMessage;
    bus->lastMessageReceived = 0;
#ifdef __LOG_STATS__
    statistics::initialize(&bus->totalMessageStats);
    statistics::initialize(&bus->droppedMessageStats);
    statistics::initialize(&bus->receivedMessageStats);
    statistics::initialize(&bus->receivedDataStats);
    statistics::initialize(&bus->sendQueueStats);
    statistics::initialize(&bus->receiveQueueStats);
#endif // __LOG_STATS__
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

const CanSignalState* openxc::can::lookupSignalState(const char* name, CanSignal* signal,
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

const CanSignalState* openxc::can::lookupSignalState(int value, CanSignal* signal,
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

bool messageComparator(void* id, int index, void* messages) {
    return *((unsigned int*)id) == ((CanMessageDefinition*)messages)[index].id;
}

CanMessageDefinition* openxc::can::lookupMessage(int id,
        CanMessageDefinition* messages, int messageCount) {
    int index = lookup((void*)&id, messageComparator, (void*)messages,
            messageCount);
    if(index != -1) {
        return &messages[index];
    } else {
        return NULL;
    }
}

void openxc::can::logBusStatistics(CanBus* buses, const int busCount) {
#ifdef __LOG_STATS__
    static unsigned long lastTimeLogged;

    if(time::systemTimeMs() - lastTimeLogged > BUS_STATS_LOG_FREQUENCY_S * 1000) {
        static DeltaStatistic totalMessageStats;
        static DeltaStatistic receivedMessageStats;
        static DeltaStatistic droppedMessageStats;
        static DeltaStatistic recevedDataStats;
        static bool initializedStats = false;
        if(!initializedStats) {
            statistics::initialize(&totalMessageStats);
            statistics::initialize(&receivedMessageStats);
            statistics::initialize(&droppedMessageStats);
            statistics::initialize(&recevedDataStats);
            initializedStats = true;
        }

        int totalMessages = 0;
        int messagesReceived = 0;
        int messagesDropped = 0;
        int dataReceived = 0;
        for(int i = 0; i < busCount; i++) {
            CanBus* bus = &buses[i];

            statistics::update(&bus->receivedDataStats,
                    bus->messagesReceived * CAN_MESSAGE_TOTAL_BIT_SIZE / 8192);
            statistics::update(&bus->totalMessageStats,
                    bus->messagesReceived + bus->messagesDropped);
            statistics::update(&bus->receivedMessageStats,
                    bus->messagesReceived);
            statistics::update(&bus->droppedMessageStats, bus->messagesDropped);

            statistics::update(&bus->sendQueueStats,
                    QUEUE_LENGTH(CanMessage, &bus->sendQueue));
            statistics::update(&bus->receiveQueueStats,
                    QUEUE_LENGTH(CanMessage, &bus->receiveQueue));

            if(bus->totalMessageStats.total > 0) {
                debug("Average CAN message send queue fill percent on bus %d: %f",
                        bus->address,
                        statistics::exponentialMovingAverage(&bus->sendQueueStats)
                            / QUEUE_MAX_LENGTH(CanMessage) * 100);
                debug("CAN message send queue max fill percent on bus %d: %f",
                        bus->address,
                        (float)statistics::maximum(&bus->sendQueueStats) /
                                QUEUE_MAX_LENGTH(CanMessage) * 100)

                debug("Average CAN message receive queue fill percent on bus %d: %f",
                        bus->address,
                        statistics::exponentialMovingAverage(&bus->receiveQueueStats)
                            / QUEUE_MAX_LENGTH(CanMessage) * 100);
                debug("CAN message receive queue max fill percent on bus %d: %f",
                        bus->address,
                        (float) statistics::maximum(&bus->receiveQueueStats) /
                                QUEUE_MAX_LENGTH(CanMessage) * 100);

                debug("CAN messages received on bus %d: %d",
                        bus->address, bus->totalMessageStats.total);
                debug("CAN messages processed on bus %d: %d",
                        bus->address, bus->receivedMessageStats.total);
                debug("CAN messages dropped on bus %d: %d",
                        bus->address, bus->droppedMessageStats.total);
                if(bus->droppedMessageStats.total > 0) {
                    debug("Overall dropped message percent: %f (%d / %d)",
                            (float)bus->droppedMessageStats.total / bus->totalMessageStats.total * 100,
                            bus->droppedMessageStats.total,
                            bus->totalMessageStats.total);
                }
                debug("Overall data received on bus %d: %dKB", bus->address,
                        bus->receivedDataStats.total);
                debug("Average throughput on bus %d: %fKB / s", bus->address,
                        statistics::exponentialMovingAverage(&bus->receivedDataStats)
                            / BUS_STATS_LOG_FREQUENCY_S);
                if(bus->droppedMessageStats.total > 0) {
                    debug("Average dropped message percent on bus %d: %f", bus->address,
                            statistics::exponentialMovingAverage(&bus->droppedMessageStats) /
                            statistics::exponentialMovingAverage(&bus->totalMessageStats) * 100);
                }
            } else {
                debug("No messages received on bus %d", bus->address);
            }

            totalMessages += bus->totalMessageStats.total;
            messagesReceived += bus->messagesReceived;
            messagesDropped += bus->messagesDropped;
            dataReceived += bus->receivedDataStats.total;
        }
        statistics::update(&totalMessageStats, totalMessages);
        statistics::update(&receivedMessageStats, messagesReceived);
        statistics::update(&droppedMessageStats, messagesDropped);
        statistics::update(&recevedDataStats, dataReceived);

        if(totalMessageStats.total > 0) {
            debug("Total CAN messages dropped on all buses: %d",
                    droppedMessageStats.total);
            debug("Average message drop rate across all buses: %d msgs / s",
                    (int)(statistics::exponentialMovingAverage(&droppedMessageStats)
                        / BUS_STATS_LOG_FREQUENCY_S));
            debug("Dropped message percent across all buses: %f (%d / %d)",
                    (float)droppedMessageStats.total / totalMessageStats.total * 100,
                    droppedMessageStats.total, totalMessageStats.total);
            debug("Total CAN messages received since startup on all buses: %d",
                    totalMessageStats.total);
            debug("Aggregate message rate across all buses since startup: %d msgs / s",
                    (int)(statistics::exponentialMovingAverage(&totalMessageStats)
                        / BUS_STATS_LOG_FREQUENCY_S));
            debug("Aggregate throughput across all buses since startup: %fKB / s",
                    statistics::exponentialMovingAverage(&recevedDataStats)
                        / BUS_STATS_LOG_FREQUENCY_S);
            debug("Average dropped message percent across all buses: %f",
                    statistics::exponentialMovingAverage(&droppedMessageStats) /
                    statistics::exponentialMovingAverage(&totalMessageStats) * 100);
        } else {
            debug("No CAN messages have been received on any bus");
        }

        lastTimeLogged = time::systemTimeMs();
    }
#endif // __LOG_STATS__
}
