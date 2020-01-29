#ifndef __CANUTIL_H__
#define __CANUTIL_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>

#include "util/timer.h"
#include "util/statistics.h"
#include "pipeline.h"
#include "cJSON.h"
#include "openxc.pb.h"

// TODO actual max is 32 but dropped to 24 for memory considerations
#define MAX_ACCEPTANCE_FILTERS 24
// TODO this takes up a ton of memory
#define MAX_DYNAMIC_MESSAGE_COUNT 12

#define CAN_MESSAGE_SIZE 8

/* Public: The type signature for a CAN signal decoder.
 *
 * A SignalDecoder transforms a raw floating point CAN signal into a number,
 * string or boolean.
 *
 * signal - The CAN signal that we are decoding.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * pipeline -  you may want to generate arbitrary additional messages for
 *      publishing.
 * value - The CAN signal parsed from the message as a raw floating point
 *      value.
 * send - An output parameter. If the decoding failed or the CAN signal should
 *      not send for some other reason, this should be flipped to false.
 *
 * Returns a decoded value in an openxc_DynamicField struct.
 */
typedef openxc_DynamicField (*SignalDecoder)(const struct CanSignal* signal, const CanSignal* signals, 
        struct SignalManager* signalManager, SignalManager* signalManagers, int signalCount,
        openxc::pipeline::Pipeline* pipeline, float value, bool* send);

/* Public: The type signature for a CAN signal encoder.
 *
 * A SignalEncoder transforms a number, string or boolean into a raw floating
 * point value that fits in the CAN signal.
 *
 * signal - The CAN signal to encode.
 * value - The dynamic field to encode.
 * send - An output parameter. If the encoding failed or the CAN signal should
 * not be encoded for some other reason, this will be flipped to false.
 */
typedef uint64_t (*SignalEncoder)(const struct CanSignal* signal,
        openxc_DynamicField* value, bool* send);

/* Public: The ID format for a CAN message.
 *
 * STANDARD - standard 11-bit CAN arbitration ID.
 * EXTENDED - an extended frame, with a 29-bit arbitration ID.
 */
enum CanMessageFormat {
    STANDARD,
    EXTENDED,
};
typedef enum CanMessageFormat CanMessageFormat;

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
 * decoder     - An optional function to decode a signal from the bus to a human
 *      readable value. If NULL, the default numerical decoder is used.
 * encoder     - An optional function to encode a signal value to be written to
 *                CAN into a byte array. If NULL, the default numerical encoder
 *                is used.
 * received    - True if this signal has ever been received.
 * lastValue   - The last received value of the signal. If 'received' is false,
 *      this value is undefined.
 */
struct CanSignal {
    const struct CanMessageDefinition* message;
    const char* genericName;
    uint8_t bitPosition;
    uint8_t bitSize;
    float factor;
    float offset;
    float minValue;
    float maxValue;
    float frequency;
    bool sendSame;
    bool forceSendChanged;
    const CanSignalState* states;
    uint8_t stateCount;
    bool writable;
    SignalDecoder decoder;
    SignalEncoder encoder;
};
typedef struct CanSignal CanSignal;

struct SignalManager {
    const CanSignal* signal;
    openxc::util::time::FrequencyClock frequencyClock;
    bool received;
    float lastValue;
};
typedef struct SignalManager SignalManager;

/* Public: The definition of a CAN message. This includes a lot of metadata, so
 * to save memory this struct should not be used for storing incoming and
 * outgoing CAN messages.
 *
 * bus - A pointer to the bus this message is on.
 * id - The ID of the message.
 * format - the format of the message's ID.
 * frequencyClock - an optional frequency clock to control the output of this
 *      message, if sent raw, or simply to mark the max frequency for custom
 *      handlers to retrieve.
 * forceSendChanged - If true, regardless of the frequency, it will send CAN
 *      message if it has changed when using raw passthrough.
 * lastValue - The last received value of the message. Defaults to undefined.
 *      This is required for the forceSendChanged functionality, as the stack
 *      needs to compare an incoming CAN message with the previous frame.
 */
struct CanMessageDefinition {
    struct CanBus* bus;
    uint32_t id;
    CanMessageFormat format;
    openxc::util::time::FrequencyClock frequencyClock;
    bool forceSendChanged;
    uint8_t lastValue[CAN_MESSAGE_SIZE];
};
typedef struct CanMessageDefinition CanMessageDefinition;

/* A compact representation of a single CAN message, meant to be used in in/out
 * buffers.
 *
 * id - The ID of the message.
 * format - the format of the message's ID.
 * data  - The message's data field.
 * length - the length of the data array (max 8).
 */
struct CanMessage {
    uint32_t id;
    CanMessageFormat format;
    uint8_t data[CAN_MESSAGE_SIZE];
    uint8_t length;
};
typedef struct CanMessage CanMessage;

QUEUE_DECLARE(CanMessage, 8);

/* Private: An entry in the list of acceptance filters for each CanBus.
 *
 * This struct is meant to be used with a LIST type from <sys/queue.h>.
 *
 * filter - the value for the CAN acceptance filter.
 * activeUserCount - The number of active consumers of this filter's messages.
 *      When 0, this filter can be removed.
 * format - the format of the ID for the filter.
 */
struct AcceptanceFilterListEntry {
    uint32_t filter;
    uint8_t activeUserCount;
    CanMessageFormat format;
    LIST_ENTRY(AcceptanceFilterListEntry) entries;
};

/* Private: A type of list containing CAN acceptance filters.
 */
LIST_HEAD(AcceptanceFilterList, AcceptanceFilterListEntry);

struct CanMessageDefinitionListEntry {
    CanMessageDefinition definition;
    LIST_ENTRY(CanMessageDefinitionListEntry) entries;
};
LIST_HEAD(CanMessageDefinitionList, CanMessageDefinitionListEntry);

/* Public: A container for a CAN module paried with a certain bus.
 *
 * There are three things that control the operating mode of the CAN controller:
 *
 *     - Should arbitrary CAN message writes be allowed? See rawWritable.
 *     - Should translated, simple vehicle message writes be allowed? See the
 *         'writable' field in signals defined for this bus.
 *     - Should it be in loopback mode? See loopback.
 *
 * speed - The bus speed in bits per second (e.g. 500000)
 * address - The address or ID of this node
 * maxMessageFrequency - the default maximum frequency for all CAN messages when
 *      using the raw passthrough mode. To put no limit on the frequency, set
 *      this to 0.
 * rawWritable - True if this CAN bus connection should allow raw CAN messages
 *      writes. This is independent from the CanSignal 'writable' option, which
 *      can be set to still allow translated writes back to this bus.
 * passthroughCanMessages - True if low-level CAN messages should be send to the
 *      output interface, not just signals as simple vehicle messages.
 * bypassFilters - a boolean to indicate if the CAN controller's
 *      acceptance filter should be in bypass mode. Set to true to receive all
 *      messages for this bus, regardless of what is defined in the
 *      acceptanceFilters list. The AF will automatically be bypassed if there
 *      are no acceptance filters configured.
 * loopback - True if the controller should be configured in loopback mode, so
 *         all sent messages are received immediately on that same controller.
 *
 * acceptanceFilters - a list of active acceptance filters for this bus.
 * freeAcceptanceFilters - a list of available slots for acceptance filters.
 * acceptanceFilterEntries - static memory allocated for entires in the
 *      acceptanceFilters and freeAcceptanceFilters list.
 * dynamicMessages - a list of CAN message IDs ever received on this bus. This
 *      is used for message frequency control and metrics.
 * freeMessageDefinitions - a list of available slots for dynamic message
 *      definitions.
 * definitionEntries - static memory allocated for entires in the
 *      dynamicMessages and freeMessageDefinitions list.
 * writeHandler - a function that actually writes out a CanMessage object to the
 *      CAN interface (implementation is platform specific);
 * lastMessageReceived - the time (in ms) when the last CAN message was
 *      received. If no message has been received, it should be 0.
 * messagesReceived - A count of the number of CAN messages received.
 * messagesDropped - A count of the number of CAN messages we knowingly dropped
 * - i.e. we received an interrupt with a new CAN message but the incoming CAN
 *   message queue was full.
 * sendQueue - a queue of CanMessage instances that need to be written to CAN.
 * receiveQueue - a queue of messages received from CAN that have yet to be
 *      translated.
 */
struct CanBus {
    unsigned int speed;
    short address;
    float maxMessageFrequency;
    bool rawWritable;
    bool passthroughCanMessages;
    bool bypassFilters;
    bool loopback;

    // Private
    AcceptanceFilterList acceptanceFilters;
    AcceptanceFilterList freeAcceptanceFilters;
    AcceptanceFilterListEntry acceptanceFilterEntries[MAX_ACCEPTANCE_FILTERS];
    CanMessageDefinitionList dynamicMessages;
    CanMessageDefinitionList freeMessageDefinitions;
    CanMessageDefinitionListEntry definitionEntries[MAX_DYNAMIC_MESSAGE_COUNT];
    bool (*writeHandler)(CanBus*, CanMessage*);
    unsigned long lastMessageReceived;
    unsigned int messagesReceived;
    unsigned int messagesDropped;

    // TODO These are unnecessary if you aren't calculating metrics, and they do
    // take up a bit of memory.
    openxc::util::statistics::DeltaStatistic totalMessageStats;
    openxc::util::statistics::DeltaStatistic droppedMessageStats;
    openxc::util::statistics::DeltaStatistic receivedMessageStats;
    openxc::util::statistics::DeltaStatistic receivedDataStats;
    openxc::util::statistics::Statistic sendQueueStats;
    openxc::util::statistics::Statistic receiveQueueStats;

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

/* Public: The type signature for a function to handle a custom OpenXC command.
 *
 * name - the name of the received command.
 * value - the value of the received command, in a DynamicField. The actual type
 *      may be a number, string or bool.
 * event - an optional event from the received command, in a DynamicField. The
 *      actual type may be a number, string or bool.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 */
typedef void (*CommandHandler)(const char* name, openxc_DynamicField* value,
        openxc_DynamicField* event, const CanSignal* signals, int signalCount);

/* Public: The structure to represent a supported custom OpenXC command.
 *
 * For completely customized CAN commands without a 1-1 mapping between an
 * OpenXC message from the host and a CAN signal, you can define the name of the
 * command and a custom function to handle it in the VI. An example is
 * the "turn_signal_status" command in OpenXC, which has a value of "left" or
 * "right". The vehicle may have separate CAN signals for the left and right
 * turn signals, so you will need to implement a custom command handler to send
 * the correct signals.
 *
 * Command handlers are also useful if you want to trigger multiple CAN messages
 * or signals from a signal OpenXC message.
 *
 * genericName - The name of the command.
 * handler - An function to process the received command's data and perform some
 *      action.
 */
typedef struct {
    const char* genericName;
    CommandHandler handler;
} CanCommand;

/* Private: Initialize the CAN controller.
 *
 * This function must be defined for each platform - it's hardware dependent.
 *
 * bus - A CanBus struct defining the bus's metadata for initialization.
 * writable - configure the controller in a writable mode. If false, it will be
 *      configured as "listen only" and will not allow writes or even CAN ACKs.
 * buses - An array of all CAN buses.
 * busCount - The length of the buses array.
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

/* Private: De-initialize the CAN controller.
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
const CanSignal* lookupSignal(const char* name, const CanSignal* signals, int signalCount);

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
const CanSignal* lookupSignal(const char* name, const CanSignal* signals, int signalCount,
        bool writable);

SignalManager* lookupSignalManagerDetails(const char* signalName, SignalManager* signalManagers, int signalCount);

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
 *
 * Returns a pointer to the CanSignalState if found, otherwise NULL.
 */
const CanSignalState* lookupSignalState(const char* name, const CanSignal* signal);

/* Public: Look up a CanSignalState for a CanSignal by its numerical value.
 * Use this to find the string equivalent value to write over USB when a float
 * value is received from CAN.
 *
 * value - The numerical value equivalent for the state.
 * name - The string name of the desired signal state.
 * signal - The CanSignal that should include this state.
 *
 * Returns a pointer to the CanSignalState if found, otherwise NULL.
 */
const CanSignalState* lookupSignalState(int value, const CanSignal* signal);

/* Public: Search all predefined and dynamically configured CAN messages for one
 * matching the given ID.
 *
 * bus - The CanBus to search for the message.
 * id - The ID of the CAN message.
 * format - The format of the ID of the message.
 * predefinedMessages - The list of predefined CAN messages to search.
 * predefinedMessageCount - The length of the predefined messages array.
 *
 * Returns a pointer to the CanMessage if found, otherwise NULL.
 */
CanMessageDefinition* lookupMessageDefinition(CanBus* bus, uint32_t id,
        CanMessageFormat format,
        const CanMessageDefinition* predefinedMessages,
        int predefinedMessageCount);

/* Public: Search all active CAN buses for one using the given controller
 * address.
 *
 * address - The CAN controller address to search for, e.g. 1 or 2.
 * buses - An array of all active CanBus instances.
 * busCount - The length of the buses array.
 */
CanBus* lookupBus(uint8_t address, CanBus* buses, const int busCount);

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
 * format - the format of the ID of the message.
 * predefinedMessages - The list of predefined CAN messages to search for an
 *      existing match.
 * predefinedMessageCount - The length of the predefined messages array.
 *
 * Returns true if the message definition was registered successfully.
 */
bool registerMessageDefinition(CanBus* bus, uint32_t id,
        CanMessageFormat format,
        const CanMessageDefinition* predefinedMessages,
        int predefinedMessageCount);

/* Public: The opposite of registerMessageDefinition(...) - removes a definition
 * if it exists for the ID on the given bus.
 *
 * bus - The CanBus to search for the message definition.
 * id - The ID of the CAN message.
 * format - the format of the ID of the message.
 *
 * Returns true if the message was found and unregistered successfully. If the
 * message was not registered, returns false.
 */
bool unregisterMessageDefinition(CanBus* bus, uint32_t id,
        CanMessageFormat format);

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
 * format - the format of the ID for the new filter.
 * buses - An array of all active CanBus instances.
 * busCount - The length of the buses array.
 *
 * Returns true if the filter was added or already existed. Returns false if the
 * filter could not be added because of a CAN controller error or because all
 * available filter slots are taken.
 */
bool addAcceptanceFilter(CanBus* bus, uint32_t id, CanMessageFormat format,
        CanBus* buses, const int busCount);

/* Public: Remove a CAN message acceptance filter from the given bus.
 *
 * bus - The CanBus to remove the filter from.
 * id - The value of the new filter.
 * format - the format of the ID for the filter.
 * buses - An array of all active CanBus instances.
 * busCount - The length of the buses array.
 *
 * Returns true if the filter was added or already existed. Returns false if the
 * filter could not be added because of a CAN controller error or because all
 * available filter slots are taken.
 */
void removeAcceptanceFilter(CanBus* bus, uint32_t id, CanMessageFormat format,
        CanBus* buses, const int busCount);

/* Private: Apply the CAN acceptance filter configuration from software (on the
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

/* Private: Enable or disable the CAN AF, but when enabling, don't re-configure
 * anything - let the caller handle that. It will be in a blank state after
 * being enabled.
 */
bool resetAcceptanceFilterStatus(CanBus* bus, bool enabled);

/* Public: Enable or disable the CAN acceptance filter. If enabled, it will use
 * a dynamically generated list of message IDs based on the bus's message list
 * and any registered diagnostic requests.
 */
bool setAcceptanceFilterStatus(CanBus* bus, bool enabled,
        CanBus* buses, const uint busCount);

/* Public: Check if any CAN signals are configured as writable.
 *
 * bus - The bus to search for writable signal definitions.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 *
 * Returns true if any signals on the bus are writable.
 */
bool signalsWritable(CanBus* bus, const CanSignal* signals, int signalCount);

/* Public: Log transfer statistics about all active CAN buses to the debug log.
 *
 * buses - an array of active CAN buses.
 * busCount - the length of the buses array.
 */
void logBusStatistics(CanBus* buses, const int busCount);

/* Public: Perform software CAN message filtering.
 *
 * This is used primarily for the LPC17xx which has a global CAN AF - when one
 * bus has the AF off but we still want to filter on the other, we use this to
 * do software filtering based on the registered CAN messages.
 *
 * bus - The bus the message was received on.
 * messageId - the ID of the message.
 *
 * Returns true if the message should be accepted.
 */
bool shouldAcceptMessage(CanBus* bus, uint32_t messageId);

} // can
} // openxc

#endif // __CANUTIL_H__
