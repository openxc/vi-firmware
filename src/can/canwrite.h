#ifndef _CANWRITE_H_
#define _CANWRITE_H_

#include "can/canutil.h"

using openxc::can::CanCommand;

namespace openxc {
namespace can {
namespace write {

/* Public: Encode a number, string or boolean into an integer, fit for a CAN
 * signal's bitfield.
 *
 * Depending on which type the dynamic field contains, this function will use
 * the proper encoder to convert it to an integer.
 *
 * signal - The CAN signal to encode this value for.
 * field - The value to encode.
 * send - An output parameter that will be changed to false if the encode
 *      failed.
 *
 * Returns the encoded integer. If 'send' is changed to false, the field could
 * not be encoded and the return value is undefined.
 */
uint64_t encodeDynamicField(const CanSignal* signal, openxc_DynamicField* field,
                bool* send);

/* Public: Encode a boolean into an integer, fit for a CAN signal bitfield.
 *
 * This is a shortcut for encodeDynamicField(CanSignal*, openxc_DynamicField*,
 * bool*) that takes care of creating the DynamicField object for you with the
 * boolean value.
 *
 * signal - The CAN signal to encode this value for.
 * value - The boolean value to encode.
 * send - An output parameter that will be changed to false if the encode
 *      failed.
 *
 * Returns the encoded integer. If 'send' is changed to false, the field could
 * not be encoded and the return value is undefined.
 */
uint64_t encodeBoolean(const CanSignal* signal, bool value,
                bool* send);

/* Public: Encode a string into an integer, fit for a CAN signal's bitfield.
 *
 * Be aware that the behavior is undefined if there are multiple values assigned
 * to a single state. See https://github.com/openxc/vi-firmware/issues/185.
 *
 * This is a shortcut for encodeDynamicField(CanSignal*, openxc_DynamicField*,
 * bool*) that takes care of creating the DynamicField object for you with the
 * string state value.
 *
 * signal - The CAN signal to encode this value for.
 * value - The string state value to encode.
 * send - An output parameter that will be changed to false if the encode
 *      failed.
 *
 * Returns the encoded integer. If 'send' is changed to false, the field could
 * not be encoded and the return value is undefined.
 */
uint64_t encodeState(const CanSignal* signal, const char* state,
                bool* send);

/* Public: Encode a float into an integer, fit for a CAN signal's bitfield.
 *
 * This is a shortcut for encodeDynamicField(CanSignal*, openxc_DynamicField*,
 * bool*) that takes care of creating the DynamicField object for you with the
 * float value.
 *
 * signal - The CAN signal to encode this value for.
 * value - The float value to encode.
 * send - An output parameter that will be changed to false if the encode
 *      failed.
 *
 * Returns the encoded integer. If 'send' is changed to false, the field could
 * not be encoded and the return value is undefined.
 */
uint64_t encodeNumber(const CanSignal* signal, float value, bool* send);

/* Public: Write a value into a CAN signal in the destination buffer.
 *
 * signal - The CAN signal to write, including the bit position and bit size.
 * encodedValue - The encoded integer value to write into the CAN signal.
 * destination - The destination buffer.
 * length - The length of the destination buffer.
 */
void buildMessage(const CanSignal* signal, uint64_t encodedValue,
                uint8_t destination[], size_t length);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Using the provided CanSignal and SignalEncoder function, convert the value
 * into a numerical value appropriate for the CAN signal.
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
bool encodeAndSendSignal(const CanSignal* signal, openxc_DynamicField* value,
        SignalEncoder writer, bool force);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Just like the above function sendSignal() that accepts a writer function,
 * but uses the CanSignal's value for "writeHandler" instead.
 *
 * See above for argument descriptions.
 */
bool encodeAndSendSignal(const CanSignal* signal, openxc_DynamicField* value, bool force);

// value is already encoded
/* Public: Write a previously encoded CAN signal to a CAN message on the bus.
 *
 * Similar to encodeAndSendSignal(), but the value is already encoded.
 *
 * signal - The CanSignal to send.
 * value - The encoded value to send in the signal.
 * force - true if the signals should be sent regardless of the writable status
 *         in the CAN message structure.
 */
bool sendEncodedSignal(const CanSignal* signal, uint64_t value, bool force);

/* Public: Three shortcut functions to encode and send a signal without manually
 * creating an openxc_DynamicField.
 */
bool encodeAndSendBooleanSignal(const CanSignal* signal, bool value, bool force);
bool encodeAndSendStateSignal(const CanSignal* signal, const char* value, bool force);
bool encodeAndSendNumericSignal(const CanSignal* signal, float value, bool force);

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
 * immediately.
 *
 * You should usually use enqueueMessage, unless you absolutely need the message
 * written to the bus right now.
 *
 * bus - The CAN bus to send the message on.
 * request - the CanMessage message to send.
 *
 * Returns true if the message was sent successfully.
 */
bool sendCanMessage(CanBus* bus, CanMessage* request);

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
bool sendMessage(CanBus* bus, CanMessage* request);

} // namespace write
} // namespace can
} // namespace openxc

#endif // _CANWRITE_H_
