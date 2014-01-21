#ifndef __CANUTIL_H__
#define __CANUTIL_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include "util/timer.h"
#include "util/statistics.h"
#include "emqueue.h"
#include "emhashmap.h"
#include "cJSON.h"

#define MAX_ACCEPTANCE_FILTERS 32

/* Public: A state encoded (SED) signal's mapping from numerical values to
 * OpenXC state names.
 *
 * value - The integer value of the state on the CAN bus.
 * name  - The corresponding string name for the state in OpenXC.
 */
struct CanSignalState {
    const int value;
    const char* name;
};
typedef struct CanSignalState CanSignalState;

/* Public: A CAN signal to decode from the bus and output over USB.
 *
 * message     - The message this signal is a part of.
 * genericName - The name of the signal to be output over USB.
 * bitPosition - The starting bit of the signal in its CAN message (assuming
 *               non-inverted bit numbering, i.e. the most significant bit of
 *               each byte is 0)
 * bitSize     - The width of the bit field in the CAN message.
 * factor      - The final value will be multiplied by this factor. Use 1 if you
 *               don't need a factor.
 * offset      - The final value will be added to this offset. Use 0 if you
 *               don't need an offset.
 * minValue    - The minimum value for the processed signal.
 * maxValue    - The maximum value for the processed signal.
 * frequencyClock - A FrequencyClock struct to control the maximum frequency to
 *              process and send this signal. To process every value, set the
 *              clock's frequency to 0.
 * sendSame    - If true, will re-send even if the value hasn't changed.
 * forceSendChanged - If true, regardless of the frequency, it will send the
 *              value if it has changed.
 * states      - An array of CanSignalState describing the mapping
 *               between numerical and string values for valid states.
 * stateCount  - The length of the states array.
 * writable    - True if the signal is allowed to be written from the USB host
 *               back to CAN. Defaults to false.
 * writeHandler - An optional function to encode a signal value to be written to
 *                CAN into a uint64_t. If null, the default encoder is used.
 * received    - Marked true if this signal has ever been received.
 * lastValue   - The last received value of the signal. Defaults to undefined.
 */
struct CanSignal {
    struct CanMessageDefinition* message;
    const char* genericName;
    uint8_t bitPosition;
    uint8_t bitSize;
    float factor;
    float offset;
    float minValue;
    float maxValue;
    openxc::util::time::FrequencyClock frequencyClock;
    bool sendSame;
    bool forceSendChanged;
    const CanSignalState* states;
    uint8_t stateCount;
    bool writable;
    uint64_t (*writeHandler)(struct CanSignal*, struct CanSignal*, int, cJSON*,
            bool*);
    bool received;
    float lastValue;
};
typedef struct CanSignal CanSignal;

/* Public: The definition of a CAN message. This includes a lot of metadata, so
 * to save memory this struct should not be used for storing incoming and
 * outgoing CAN messages.
 *
 * bus - A pointer to the bus this message is on.
 * id - The ID of the message.
 * frequencyClock - an optional frequency clock to control the output of this
 *      message, if sent raw, or simply to mark the max frequency for custom
 *      handlers to retrieve.
 * forceSendChanged - If true, regardless of the frequency, it will send CAN
 *      message if it has changed when using raw passthrough.
 * lastValue - The last received value of the message. Defaults to undefined.
 */
struct CanMessageDefinition {
    struct CanBus* bus;
    uint32_t id;
    openxc::util::time::FrequencyClock frequencyClock;
    bool forceSendChanged;
    uint64_t lastValue;
};
typedef struct CanMessageDefinition CanMessageDefinition;

/* A compact representation of a single CAN message, meant to be used in in/out
 * buffers.
 *
 * id - The ID of the message.
 * data  - The message's data field.
 * length - the length of the data array (max 8).
 */
struct CanMessage {
    uint32_t id;
    uint64_t data;
    uint8_t length;
};
typedef struct CanMessage CanMessage;

QUEUE_DECLARE(CanMessage,
#ifdef __LOG_STATS__
    // because the stats logging blocks the main loop much longer than normal,
    // we need to increase the incoming CAN buffer so we don't drop messages
    40
#else
    8
#endif // __LOG_STATS__
);


/* Private: An entry in the list of acceptance filters for each CanBus.
 *
 * This struct is meant to be used with a LIST type from <sys/queue.h>.
 *
 * filter - the value for the CAN acceptance filter.
 */
struct AcceptanceFilterListEntry {
    uint16_t filter;
    LIST_ENTRY(AcceptanceFilterListEntry) entries;
};

/* Private: A type of list containing CAN acceptance filters.
 */
LIST_HEAD(AcceptanceFilterList, AcceptanceFilterListEntry);

/* Public: A container for a CAN module paried with a certain bus.
 *
 * speed - The bus speed in bits per second (e.g. 500000)
 * address - The address or ID of this node
 * controller - a reference to the CAN controller in the MCU
 *      (platform dependent, needs to be casted to actual type before use).
 * maxMessageFrequency - the default maximum frequency for all CAN messages when
 *      using the raw passthrough mode. To put no limit on the frequency, set
 *      this to 0.
 * rawWritable - True if this CAN bus connection should allow raw CAN messages
 *      writes. This is independent from the CanSignal 'writable' option, which
 *      can be set to still allow translated writes back to this bus.
 * interruptHandler - a function to call by the Interrupt Service Routine when
 *      a previously registered CAN event occurs. (Only used by chipKIT, which
 *      registers a different handler per channel. LPC17xx uses the same global
 *      CAN_IRQHandler.
 * acceptanceFilters - a list of active acceptance filters for this bus.
 * freeAcceptanceFilters - a list of available slots for acceptance filters.
 * acceptanceFilterEntries - static memory allocated for entires in the
 *      acceptanceFilters and freeAcceptanceFilters list.
 * writeHandler - a function that actually writes out a CanMessage object to the
 *      CAN interface (implementation is platform specific);
 * lastMessageReceived - the time (in ms) when the last CAN message was
 *      received. If no message has been received, it should be 0.
 * sendQueue - a queue of CanMessage instances that need to be written to CAN.
 * receiveQueue - a queue of messages received from CAN that have yet to be
 *      translated.
 */
struct CanBus {
    unsigned int speed;
    short address;
    void* controller;
    unsigned short maxMessageFrequency;
    bool rawWritable;
    void (*interruptHandler)();
    AcceptanceFilterList acceptanceFilters;
    AcceptanceFilterList freeAcceptanceFilters;
    AcceptanceFilterListEntry acceptanceFilterEntries[MAX_ACCEPTANCE_FILTERS];
    HashMap* dynamicMessages;
    bool (*writeHandler)(const CanBus*, const CanMessage*);
    unsigned long lastMessageReceived;
    unsigned int messagesReceived;
    unsigned int messagesDropped;
#ifdef __LOG_STATS__
    openxc::util::statistics::DeltaStatistic totalMessageStats;
    openxc::util::statistics::DeltaStatistic droppedMessageStats;
    openxc::util::statistics::DeltaStatistic receivedMessageStats;
    openxc::util::statistics::DeltaStatistic receivedDataStats;
    openxc::util::statistics::Statistic sendQueueStats;
    openxc::util::statistics::Statistic receiveQueueStats;
#endif // __LOG_STATS__
    QUEUE_TYPE(CanMessage) sendQueue;
    QUEUE_TYPE(CanMessage) receiveQueue;
};
typedef struct CanBus CanBus;

/** Public: A parent wrapper for a particular set of CAN messages and associated
 *  CAN buses(e.g. a vehicle or program).
 *
 *  index - A numerical ID for the message set, ideally the index in an array
 *      for fast lookup
 *  name - The name of the message set.
 *  busCount - The number of CAN buses defined for this message set.
 *  messageCount - The number of CAN messages (across all buses) defined for
 *      this message set.
 *  signalCount - The number of CAN signals (across all messages) defined for
 *      this message set.
 *  commandCount - The number of CanCommmands defined for this message set.
 */
typedef struct {
    uint8_t index;
    const char* name;
    uint8_t busCount;
    unsigned short messageCount;
    unsigned short signalCount;
    unsigned short commandCount;
} CanMessageSet;

namespace openxc {
namespace can {

extern const int CAN_ACTIVE_TIMEOUT_S;

/* Public: The function definition for completely custom OpenXC command
 * handlers.
 *
 * name - the name field in the message received over USB.
 * value - the value of the message, parsed by the cJSON library and able to be
 *         read as a string, boolean, float or int.
 * event - an optional event, may be null if the OpenXC message didn't include
 *          it.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 *
 * Returns true if the command caused something to be sent over CAN.
 */
typedef bool (*CommandHandler)(const char* name, cJSON* value, cJSON* event,
        CanSignal* signals, int signalCount);

/* Public: A command to read from USB and possibly write back to CAN.
 *
 * For completely customized CAN commands without a 1-1 mapping between an
 * OpenXC message from the host and a CAN signal, you can define the name of the
 * command and a custom function to handle it in the translator. An example is
 * the "turn_signal_status" command in OpenXC, which has a value of "left" or
 * "right". The vehicle may have separate CAN signals for the left and right
 * turn signals, so you will need to implement a custom command handler to
 *
 * genericName - The name of message received over USB.
 * handler - An function to actually process the received command's value
 *                and write it to CAN in the proper signals.
 */
typedef struct {
    const char* genericName;
    CommandHandler handler;
} CanCommand;

/* Public: Initialize the CAN controller.
 *
 * This function must be defined for each platform - it's hardware dependent.
 *
 * bus - A CanBus struct defining the bus's metadata for initialization.
 * writable - configure the controller in a writable mode. If False, it will be
 *      configured as "listen only" and will not allow writes or even CAN ACKs.
 */
void initialize(CanBus* bus, bool writable, CanBus* buses, const int busCount);

/* Public: Free any memory associated with the CanBus.
 *
 * This doesn't run any deinit routines, just frees memory if you need to
 * re-initialize at runtime.
 *
 * bus - The bus to destroy.
 */
void destroy(CanBus* bus);

/* Public: De-initialize the CAN controller.
 *
 * This function must be defined for each platform - it's hardware dependent.
 *
 * bus - A CanBus struct defining the bus's metadata for initialization.
 */
void deinitialize(CanBus* bus);

/* Public: Perform platform-agnostic CAN initialization.
 */
void initializeCommon(CanBus* bus);

/* Public: Check if the device is connected to an active CAN bus, i.e. it's
 * received a message in the recent past.
 *
 * Returns true if a message was received on the CAN bus within
 * CAN_ACTIVE_TIMEOUT_S seconds.
 */
bool busActive(CanBus* bus);

/* Public: Look up the CanSignal representation of a signal based on its generic
 * name. The signal may or may not be writable - the first result will be
 * returned.
 *
 * name - The generic, OpenXC name of the signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 *
 * Returns a pointer to the CanSignal if found, otherwise NULL.
 */
CanSignal* lookupSignal(const char* name, CanSignal* signals, int signalCount);

/* Public: Look up the CanSignal representation of a signal based on its generic
 * name.
 *
 * name - The generic, OpenXC name of the signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * writable - If true, only consider signals that are writable as candidates.
 *
 * Returns a pointer to the CanSignal if found, otherwise NULL.
 */
CanSignal* lookupSignal(const char* name, CanSignal* signals, int signalCount,
        bool writable);

/* Public: Look up the CanCommand representation of a command based on its
 * generic name.
 *
 * name - The generic, OpenXC name of the command.
 * commands - The list of all commands.
 * commandCount - The length of the commands array.
 *
 * Returns a pointer to the CanSignal if found, otherwise NULL.
 */
CanCommand* lookupCommand(const char* name, CanCommand* commands,
        int commandCount);

/* Public: Look up a CanSignalState for a CanSignal by its textual name. Use
 * this to find the numerical value to write back to CAN when a string state is
 * received from the user.
 *
 * name - The string name of the desired signal state.
 * signal - The CanSignal that should include this state.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 *
 * Returns a pointer to the CanSignalState if found, otherwise NULL.
 */
const CanSignalState* lookupSignalState(const char* name, CanSignal* signal,
        CanSignal* signals, int signalCount);

/* Public: Look up a CanSignalState for a CanSignal by its numerical value.
 * Use this to find the string equivalent value to write over USB when a float
 * value is received from CAN.
 *
 * value - the numerical value equivalent for the state.
 * name - The string name of the desired signal state.
 * signal - The CanSignal that should include this state.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 *
 * Returns a pointer to the CanSignalState if found, otherwise NULL.
 */
const CanSignalState* lookupSignalState(int value, CanSignal* signal,
        CanSignal* signals, int signalCount);

/* Public: Search all predefined and dynamically configured CAN messages for one
 * matching the given ID.
 *
 * bus - The CanBus to search for the message.
 * id - The ID of the CAN message.
 * predefinedMessages - The list of predefined CAN messages to search.
 * predefinedMessageCount - The length of the predefined messages array.
 *
 * Returns a pointer to the CanMessage if found, otherwise NULL.
 */
CanMessageDefinition* lookupMessageDefinition(CanBus* bus, uint32_t id,
        CanMessageDefinition* predefinedMessages,
        int predefinedMessageCount);

/* Public: Configure a new CAN message on the given bus.
 *
 * If the message is already registered with the bus (either as a predefined
 * definition or a dynamic), nothing will be added.
 *
 * If it is not already defined, a CanMessageDefinition will be
 * created and stored on the CanBus. This is useful for statistics, logging and
 * potentially changing CAN acceptance filters on the fly (although that is not
 * supported at the moment). The "forceSendChanged" will be true for the new
 * message definition.
 *
 * bus - The CanBus to register the message on.
 * id - The ID of the new CAN message definition.
 * predefinedMessages - The list of predefined CAN messages to search for an
 *      existing match.
 * predefinedMessageCount - The length of the predefined messages array.
 *
 * Returns true if the message definition was registered successfully.
 */
bool registerMessageDefinition(CanBus* bus, uint32_t id,
        CanMessageDefinition* predefinedMessages,
        int predefinedMessageCount);

/* Public: The opposite of registerMessageDefinition(...) - removes a definition
 * if it exists for the ID on the given bus.
 *
 * bus - The CanBus to search for the message definition.
 * id - The ID of the CAN message.
 *
 * Returns true if the message was found and unregistered successfully. If the
 * message was not registered, returns false.
 */
bool unregisterMessageDefinition(CanBus* bus, uint32_t id);

/* Public: Based on the predefined CAN messages for a bus, add the required
 * CAN acceptance filters to receive all messages.
 *
 * This will find messages in the messages array configured on the given bus and
 * add an acceptance filter for the message ID.
 *
 * This function is *not* platform specific - it uses the
 * addAcceptanceFilter(...) function.
 *
 * bus - The CanBus to initialize the default acceptance filters for.
 * messages - An array of all active CAN messages definitions.
 * messageCount - The length of the messages array.
 * buses - An array of all active CanBus instances.
 * busCount - The length of the buses array.
 *
 * Returns true if the acceptance filters were all configured successfully.
 */
bool configureDefaultFilters(CanBus* bus, const CanMessageDefinition* messages, 
        const int messageCount, CanBus* buses, const int busCount);

/* Public: Configure a new CAN message acceptance filter on the given bus.
 *
 * bus - The CanBus to initialize the filter on.
 * id - The value of the new filter.
 * buses - An array of all active CanBus instances.
 * busCount - The length of the buses array.
 *
 * Returns true if the filter was added or already existed. Returns false if the
 * filter could not be added because of a CAN controller error or because all
 * available filter slots are taken.
 */
bool addAcceptanceFilter(CanBus* bus, uint32_t id, CanBus* buses, 
        const int busCount);

/* Public: Remove a CAN message acceptance filter from the given bus.
 *
 * buses - An array of all active CanBus instances.
 * busCount - The length of the buses array.
 * bus - The CanBus to remove the filter from.
 * id - The value of the new filter.
 *
 * Returns true if the filter was added or already existed. Returns false if the
 * filter could not be added because of a CAN controller error or because all
 * available filter slots are taken.
 */
void removeAcceptanceFilter(CanBus* bus, uint32_t id, CanBus* buses, 
        const int busCount);

/* Public: Apply the CAN acceptance filter configuration from software (on the
 * CanBus struct) to the actual hardware CAN controllers.
 *
 * This function must be defined for each platform - it's hardware dependent.
 *
 * The 2 platforms that this firmware is compiled for at the moment (PIC32 and
 * LPC17xx) don't very well support adding or removing a single CAN acceptance
 * filter on the fly. It is much easier to completely erase and reset the entire
 * table. There is certainly a way to do it on both, but it would involve quite
 * a bit more code - e.g. the LPC17xx's acceptance filter table must be sorted
 * in ascending order, both platforms have static slots for filters that we
 * would have to keep track of, etc.
 *
 * With that in mind, it's easier to have this function to say that when called,
 * the CAN controller's AF table should mirror the active filter list
 * (CanBus.AcceptanceFilterList).
 *
 * buses - An array of all active CanBus instances.
 * busCount - The length of the buses array.
 *
 * Returns true if the acceptance filters were all configured properly.
 */
bool updateAcceptanceFilterTable(CanBus* buses, const int busCount);

/* Public: Check if any CAN signals are configured as writable.
 *
 * bus - The bus to search for writable signal definitions.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 *
 * Returns true if any signals on the bus are writable.
 */
bool signalsWritable(CanBus* bus, CanSignal* signals, int signalCount);

/* Public: Log transfer statistics about all active CAN buses to the debug log.
 *
 * buses - an array of active CAN buses.
 * busCount - the length of the buses array.
 */
void logBusStatistics(CanBus* buses, const int busCount);

} // can
} // openxc

#endif // __CANUTIL_H__
