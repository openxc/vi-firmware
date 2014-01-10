#ifndef __CANUTIL_H__
#define __CANUTIL_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include "util/bitfield.h"
#include "util/timer.h"
#include "util/statistics.h"
#include "emqueue.h"
#include "emhashmap.h"
#include "cJSON.h"

#ifdef __LPC17XX__
#include "platform/lpc17xx/canutil_lpc17xx.h"
#endif // __LPC17XX__

#define MAX_ACCEPTANCE_FILTERS 32

// TODO These structs are defined outside of the openxc::can namespace because
// we're not able to used namespaced types with emqueue.

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
    uint64_t (*writeHandler)(struct CanSignal*, struct CanSignal*, int, cJSON*, bool*);
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
 */
struct CanMessage {
    uint32_t id;
    uint64_t data;
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


struct AcceptanceFilterListEntry {
    uint16_t filter;
    LIST_ENTRY(AcceptanceFilterListEntry) entries;
};

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
    bool (*writeHandler)(CanBus*, CanMessage);
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
 * handler - An function to actually process the recieved command's value
 *                and write it to CAN in the proper signals.
 */
typedef struct {
    const char* genericName;
    CommandHandler handler;
} CanCommand;

/* Public: Initialize the CAN controller.
 *
 * bus - A CanBus struct defining the bus's metadata for initialization.
 * writable - configure the controller in a writable mode. If False, it will be
 *      configured as "listen only" and will not allow writes or even CAN ACKs.
 */
void initialize(CanBus* bus, bool writable);

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
CanCommand* lookupCommand(const char* name, CanCommand* commands, int commandCount);

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

/* Public: Look up the CanMessage representation of a message based on its ID.
 *
 * id - The ID of the CAN message.
 * messages - The list of all CAN messages.
 * messageCount - The length of the messages array.
 *
 * TODO mention dynamic messages, update docs
 *
 * Returns a pointer to the CanMessage if found, otherwise NULL.
 */
CanMessageDefinition* lookupMessage(int id, CanMessageDefinition* messages,
        int messageCount);

CanMessageDefinition* lookupMessageDefinition(CanBus* bus, uint32_t id,
        CanMessageDefinition* predefinedMessages,
        int predefinedMessageCount);

bool registerMessageDefinition(CanBus* bus, uint32_t id,
        CanMessageDefinition* predefinedMessages,
        int predefinedMessageCount);

bool unregisterMessageDefinition(CanBus* bus, uint32_t id);

/* Public: Check if any CAN signals are configured as writable.
 *
 * bus - make sure the signals is found on this bus.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 */
bool signalsWritable(CanBus* bus, CanSignal* signals, int signalCount);

/* Public: Log transfer statistics about all active CAN buses to the debug log.
 *
 * buses - an array of active CAN buses.
 * busCount - the length of the buses array.
 */
void logBusStatistics(CanBus* buses, const int busCount);

bool configureDefaultFilters(CanBus* bus, const CanMessageDefinition* message,
        const int messageCount);

bool addAcceptanceFilter(CanBus* bus, uint32_t id);

void removeAcceptanceFilter(CanBus* bus, uint32_t id);

bool setAcceptanceFilterStatus(CanBus* bus, bool enabled);

bool updateAcceptanceFilterTable(CanBus* bus);

} // can
} // openxc

#endif // __CANUTIL_H__
