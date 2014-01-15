#include "can/canutil.h"
#include "can/canwrite.h"
#include "util/timer.h"
#include "util/statistics.h"
#include "util/log.h"

#define BUS_STATS_LOG_FREQUENCY_S 15
#define CAN_MESSAGE_TOTAL_BIT_SIZE 128
#define DYNAMIC_MESSAGE_MAP_CAPACITY 25

namespace time = openxc::util::time;
namespace statistics = openxc::util::statistics;

using openxc::util::log::debug;
using openxc::util::statistics::DeltaStatistic;

const int openxc::can::CAN_ACTIVE_TIMEOUT_S = 30;

void openxc::can::initializeCommon(CanBus* bus) {
    debug("Initializing CAN node %d...", bus->address);
    QUEUE_INIT(CanMessage, &bus->receiveQueue);
    QUEUE_INIT(CanMessage, &bus->sendQueue);

    LIST_INIT(&bus->acceptanceFilters);
    LIST_INIT(&bus->freeAcceptanceFilters);
    for(size_t i = 0; i < MAX_ACCEPTANCE_FILTERS; i++) {
        LIST_INSERT_HEAD(&bus->freeAcceptanceFilters,
                &bus->acceptanceFilterEntries[i], entries);
    }

    bus->writeHandler = openxc::can::write::sendMessage;
    bus->lastMessageReceived = 0;
    // TODO could create this on the fly since most of the time we won't use it
    bus->dynamicMessages = emhashmap_create(DYNAMIC_MESSAGE_MAP_CAPACITY);
#ifdef __LOG_STATS__
    statistics::initialize(&bus->totalMessageStats);
    statistics::initialize(&bus->droppedMessageStats);
    statistics::initialize(&bus->receivedMessageStats);
    statistics::initialize(&bus->receivedDataStats);
    statistics::initialize(&bus->sendQueueStats);
    statistics::initialize(&bus->receiveQueueStats);
#endif // __LOG_STATS__
}

void openxc::can::destroy(CanBus* bus) {
    if(bus->dynamicMessages != NULL) {
        MapIterator iterator = emhashmap_iterator(bus->dynamicMessages);
        MapEntry* next = NULL;
        while((next = emhashmap_iterator_next(&iterator)) != NULL) {
            delete (CanMessageDefinition*)next->value;
        }
        emhashmap_destroy(bus->dynamicMessages);
    }
}

bool openxc::can::busActive(CanBus* bus) {
    return bus->lastMessageReceived != 0 &&
        time::systemTimeMs() - bus->lastMessageReceived <
            CAN_ACTIVE_TIMEOUT_S * 1000;
}

static int lookup(void* key,
        bool (*comparator)(void* key, int index, void* candidates),
        void* candidates, int candidateCount) {
    for(int i = 0; i < candidateCount; i++) {
        if(comparator(key, i, candidates)) {
            return i;
        }
    }
    return -1;
}

static bool signalStateNameComparator(void* name, int index, void* states) {
    return !strcmp((const char*)name, ((CanSignalState*)states)[index].name);
}

const CanSignalState* openxc::can::lookupSignalState(const char* name,
        CanSignal* signal, CanSignal* signals, int signalCount) {
    int index = lookup((void*)name, signalStateNameComparator,
            (void*)signal->states, signal->stateCount);
    if(index != -1) {
        return &signal->states[index];
    } else {
        return NULL;
    }
}

static bool signalStateValueComparator(void* value, int index, void* states) {
    return (*(int*)value) == ((CanSignalState*)states)[index].value;
}

const CanSignalState* openxc::can::lookupSignalState(int value,
        CanSignal* signal, CanSignal* signals, int signalCount) {
    int index = lookup((void*)&value, signalStateValueComparator,
            (void*)signal->states, signal->stateCount);
    if(index != -1) {
        return &signal->states[index];
    } else {
        return NULL;
    }
}

static bool signalComparator(void* name, int index, void* signals) {
    return !strcmp((const char*)name, ((CanSignal*)signals)[index].genericName);
}

static bool writableSignalComparator(void* name, int index, void* signals) {
    return signalComparator(name, index, signals) &&
            ((CanSignal*)signals)[index].writable;
}

CanSignal* openxc::can::lookupSignal(const char* name, CanSignal* signals,
        int signalCount, bool writable) {
    bool (*comparator)(void* key, int index, void* candidates) =
            signalComparator;
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

CanSignal* openxc::can::lookupSignal(const char* name, CanSignal* signals,
        int signalCount) {
    return lookupSignal(name, signals, signalCount, false);
}

static bool commandComparator(void* name, int index, void* commands) {
    return !strcmp((const char*)name,
            ((CanCommand*)commands)[index].genericName);
}

CanCommand* openxc::can::lookupCommand(const char* name, CanCommand* commands,
        int commandCount) {
    int index = lookup((void*)name, commandComparator, (void*)commands,
            commandCount);
    if(index != -1) {
        return &commands[index];
    } else {
        return NULL;
    }
}

/* Private: Retreive a CanMessage struct from the array given the message's ID
 * and the bus it should occur on.
 *
 * bus - The CanBus to search for the message.
 * id - The ID of the CAN message.
 * messages - The list of CAN messages to search.
 * messageCount - The length of the messages array.
 *
 * Returns a pointer to the CanMessage if found, otherwise NULL.
 */
CanMessageDefinition* lookupMessage(CanBus* bus, uint32_t id,
        CanMessageDefinition* messages, int messageCount) {
    CanMessageDefinition* message = NULL;
    for(int i = 0; i < messageCount; i++) {
        if(messages[i].bus == bus && messages[i].id == id) {
            message = &messages[i];
        }
    }
    return message;
}

CanMessageDefinition* openxc::can::lookupMessageDefinition(CanBus* bus,
        uint32_t id, CanMessageDefinition* predefinedMessages,
        int predefinedMessageCount) {
    CanMessageDefinition* message = lookupMessage(bus, id,
            predefinedMessages, predefinedMessageCount);
    if(message == NULL) {
        message = (CanMessageDefinition*)emhashmap_get(
                bus->dynamicMessages, id);
    }
    return message;
}

bool openxc::can::registerMessageDefinition(CanBus* bus, uint32_t id,
        CanMessageDefinition* predefinedMessages, int predefinedMessageCount) {
    CanMessageDefinition* message = lookupMessageDefinition(bus, id, NULL, 0);
    if(message == NULL) {
        message = new CanMessageDefinition();
        if(message != NULL) {
            message->bus = bus;
            message->id = id;
            message->frequencyClock = {bus->maxMessageFrequency};
            // TODO document that this is the default somewhere
            message->forceSendChanged = true;

            emhashmap_put(bus->dynamicMessages, id, message);
        }
    }
    return message != NULL;
}

bool openxc::can::unregisterMessageDefinition(CanBus* bus, uint32_t id) {
    CanMessageDefinition* message = (CanMessageDefinition*) emhashmap_remove(
            bus->dynamicMessages, id);
    if(message != NULL) {
        delete message;
    } else {
        return false;
    }
    return true;
}

bool openxc::can::signalsWritable(CanBus* bus, CanSignal* signals,
        int signalCount) {
    for(int i = 0; i < signalCount; i++) {
        if(bus == signals[i].message->bus && signals[i].writable) {
            return true;
        }
    }
    return false;
}

void openxc::can::logBusStatistics(CanBus* buses, const int busCount) {
#ifdef __LOG_STATS__
    static DeltaStatistic totalMessageStats;
    static DeltaStatistic receivedMessageStats;
    static DeltaStatistic droppedMessageStats;
    static DeltaStatistic receivedDataStats;
    static unsigned long lastTimeLogged;
    static bool initializedStats = false;
    if(!initializedStats) {
        statistics::initialize(&totalMessageStats);
        statistics::initialize(&receivedMessageStats);
        statistics::initialize(&droppedMessageStats);
        statistics::initialize(&receivedDataStats);
        initializedStats = true;
    }

    if(time::systemTimeMs() - lastTimeLogged >
            BUS_STATS_LOG_FREQUENCY_S * 1000) {
        unsigned int totalMessages = 0;
        unsigned int messagesReceived = 0;
        unsigned int messagesDropped = 0;
        unsigned int dataReceived = 0;
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
                debug("CAN%d Rx queue length: %d, avg: %f percent",
                        bus->address,
                        QUEUE_LENGTH(CanMessage, &bus->receiveQueue),
                        statistics::exponentialMovingAverage(
                            &bus->receiveQueueStats) /
                                QUEUE_MAX_LENGTH(CanMessage) * 100);
                debug("CAN%d Tx queue length: %d, avg: %f percent",
                        bus->address,
                        QUEUE_LENGTH(CanMessage, &bus->sendQueue),
                        statistics::exponentialMovingAverage(
                            &bus->sendQueueStats) /
                                QUEUE_MAX_LENGTH(CanMessage) * 100);
                debug("CAN%d msgs Rx: %d (%dKB)",
                        bus->address, bus->receivedMessageStats.total,
                        bus->receivedDataStats.total);
                debug("dropped: %d (avg %f percent)",
                        bus->droppedMessageStats.total,
                        statistics::exponentialMovingAverage(
                            &bus->droppedMessageStats) /
                            statistics::exponentialMovingAverage(
                                &bus->totalMessageStats) * 100);
                debug("CAN%d avg throughput: %fKB / s", bus->address,
                        statistics::exponentialMovingAverage(
                            &bus->receivedDataStats) /
                            BUS_STATS_LOG_FREQUENCY_S);
            }

            totalMessages += bus->totalMessageStats.total;
            messagesReceived += bus->messagesReceived;
            messagesDropped += bus->messagesDropped;
            dataReceived += bus->receivedDataStats.total;
        }
        statistics::update(&totalMessageStats, totalMessages);
        statistics::update(&receivedMessageStats, messagesReceived);
        statistics::update(&droppedMessageStats, messagesDropped);
        statistics::update(&receivedDataStats, dataReceived);

        if(totalMessageStats.total > 0) {
            debug("CAN total msgs Rx: %d (%dKB)",
                    receivedMessageStats.total,
                    receivedDataStats.total);
            debug("dropped: %d (avg %f percent)",
                    droppedMessageStats.total,
                    statistics::exponentialMovingAverage(&droppedMessageStats) /
                        statistics::exponentialMovingAverage(
                            &totalMessageStats) * 100);
            debug("CAN avg throughput: %fKB / s, %d msgs / s",
                    statistics::exponentialMovingAverage(&receivedDataStats)
                        / BUS_STATS_LOG_FREQUENCY_S,
                    (int)(statistics::exponentialMovingAverage(
                            &totalMessageStats) / BUS_STATS_LOG_FREQUENCY_S));
        }

        lastTimeLogged = time::systemTimeMs();

        for(int i = 0; i < busCount; i++) {
            if(QUEUE_LENGTH(CanMessage, &buses[i].receiveQueue) ==
                    QUEUE_MAX_LENGTH(CanMessage)) {
                debug("Dropped CAN messages while running stats on bus %d", i);
            }
        }
    }
#endif // __LOG_STATS__
}

bool openxc::can::configureDefaultFilters(CanBus* buses, const int busCount,
        CanBus* bus, const CanMessageDefinition* messages,
        const int messageCount) {
    uint8_t filterCount = 0;
    bool status = true;
    if(messageCount > 0) {
        for(int i = 0; i < messageCount; i++) {
            if(messages[i].bus == bus) {
                ++filterCount;
                status = status && addAcceptanceFilter(buses, busCount, bus,
                        messages[i].id);
                if(!status) {
                    debug("Couldn't add filter 0x%x to bus %d",
                            messages[i].id, bus->address);
                    break;
                }
            }
        }

        if(filterCount > 0) {
            debug("Configured %d filters for bus %d", filterCount,
                    bus->address);
        }
    }
    status &= updateAcceptanceFilterTable(buses, busCount);
    return status;
}

// TODO when we merge this branch with 'iso' this function will be duplicated
// except for the return type
static AcceptanceFilterListEntry* popListEntry(AcceptanceFilterList* list) {
    AcceptanceFilterListEntry* result = list->lh_first;
    if(result != NULL) {
        LIST_REMOVE(list->lh_first, entries);
    }
    return result;
}

bool openxc::can::addAcceptanceFilter(CanBus* buses, const int busCount,
        CanBus* bus, uint32_t id) {
    // TODO for a diagnostic request, when does a filter get removed? if a
    // request is completed and no other active requsts have the same id
    for(AcceptanceFilterListEntry* entry = bus->acceptanceFilters.lh_first;
            entry != NULL; entry = entry->entries.le_next) {
        if(entry->filter == id) {
            debug("Filter for 0x%x already exists", id);
            return true;
        }
    }

    AcceptanceFilterListEntry* availableFilter = popListEntry(
            &bus->freeAcceptanceFilters);
    if(availableFilter == NULL) {
        debug("All acceptance filter slots already taken, can't add 0x%lx",
                id);
        return false;
    }

    availableFilter->filter = id;
    LIST_INSERT_HEAD(&bus->acceptanceFilters, availableFilter, entries);
    debug("Added acceptance filter for 0x%x on bus %d", availableFilter->filter,
            bus->address);
    return updateAcceptanceFilterTable(buses, busCount);
}

void openxc::can::removeAcceptanceFilter(CanBus* buses, const int busCount,
        CanBus* bus, uint32_t id) {
    AcceptanceFilterListEntry* entry;
    for(entry = bus->acceptanceFilters.lh_first; entry != NULL;
            entry = entry->entries.le_next) {
        if(entry->filter == id) {
            break;
        }
    }

    if(entry != NULL) {
        LIST_REMOVE(entry, entries);
        updateAcceptanceFilterTable(buses, busCount);
    }
}

