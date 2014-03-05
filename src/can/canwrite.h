#ifndef _CANWRITE_H_
#define _CANWRITE_H_

#include "can/canutil.h"

using openxc::can::CanCommand;

namespace openxc {
namespace can {
namespace write {

uint64_t encodeDynamicField(const CanSignal* signal, openxc_DynamicField* field,
                bool* send);

uint64_t encodeBoolean(const CanSignal* signal, bool value, bool* send);

/* Public: Convert the string value to the correct integer value for the given
 * CAN signal and write it to the signal's bitfield.
 *
 * Be aware that the behavior is undefined if there are multiple values assigned
 * to a single state. See https://github.com/openxc/vi-firmware/issues/185.
 *
 * signal - The signal associated with the value.
 * value - The string object to write. The value should correspond to a signal
 *         state integer value.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
uint64_t encodeState(const CanSignal* signal, const char* state, bool* send);

/* Public: Write the given number to the correct bitfield for the given signal.
 *
 * signal - The signal associated with the value.
 * value - The value to write.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * TODO update docs for all of these functions.
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
uint64_t encodeNumber(const CanSignal* signal, float value, bool* send);

uint64_t buildMessage(CanSignal* signal, int encodedValue);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Using the provided CanSignal and writer function, convert the value
 * into a numerical value appropriate for the CAN signal. This may include
 * converting a string state value to its numerical equivalent, for example. The
 * writer function must know how to do this conversion (and return a fully
 * filled out uint64_t).
 *
 * signal - The CanSignal to send.
 * value - The value to send in the signal. This could be a boolean, number or
 *         string (i.e. a state value).
 * writer - A function to convert from the value to an encoded uint64_t.
 * force - true if the signals should be sent regardless of the writable status
 *         in the CAN message structure.
 *
 * Returns true if the message was sent successfully.
 */
bool encodeAndSendSignal(CanSignal* signal, openxc_DynamicField* value,
        SignalEncoder writer, bool force);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Just like the above function sendSignal() that accepts a writer function,
 * but uses the CanSignal's value for "writeHandler" instead.
 *
 * See above for argument descriptions.
 */
bool encodeAndSendSignal(CanSignal* signal, openxc_DynamicField* value, bool force);

bool sendEncodedSignal(CanSignal* signal, uint64_t value, bool force);

bool encodeAndSendBooleanSignal(CanSignal* signal, bool value, bool force);
bool encodeAndSendStateSignal(CanSignal* signal, const char* value, bool force);
bool encodeAndSendNumericSignal(CanSignal* signal, float value, bool force);

/* Public: The lowest-level API available to send a CAN message. The byte order
 * of the data is swapped, but otherwise this function queues the data to write
 * out to CAN without any additional processing.
 *
 * If the 'length' field of the CanMessage struct is 0, the message size is
 * assumed to be 8 (i.e. it will use the entire contents of the 'data' field, so
 * make sure it's all valid or zereod out!).
 *
 * bus - the bus to send the message.
 * message - the CAN message this data should be sent in. The byte order of the
 *      data will be reversed.
 */
void enqueueMessage(CanBus* bus, CanMessage* message);

/* Public: Write any queued outgoing messages to the CAN bus.
 *
 * bus - The CanBus instance that has a queued to be flushed out to CAN.
 */
void flushOutgoingCanMessageQueue(CanBus* bus);

/* Public: Write a CAN message with the given data and node ID to the bus
 * immeidately.
 *
 * You should usually use enqueueMessage, unless you absolutely need the message
 * written to the bus right now.
 *
 * bus - The CAN bus to send the message on.
 * request - the CanMessage message to send.
 *
 * Returns true if the message was sent successfully.
 */
bool sendCanMessage(const CanBus* bus, const CanMessage* request);

/* Private: Actually, finally write a CAN message with the given data and node
 * ID to the bus.
 *
 * Defined per-platform. Users should use enqueueMessage instead.
 *
 * bus - The CAN bus to send the message on.
 * request - the CanMessage message to send.
 *
 * Returns true if the message was sent successfully.
 */
bool sendMessage(const CanBus* bus, const CanMessage* request);

} // namespace write
} // namespace can
} // namespace openxc

#endif // _CANWRITE_H_
