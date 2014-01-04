#ifndef _CANWRITE_H_
#define _CANWRITE_H_

#include "can/canutil.h"

using openxc::can::CanCommand;

namespace openxc {
namespace can {
namespace write {

/* Public: Write the given number to the correct bitfield for the given signal.
 *
 * signal - The signal associated with the value.
 * signals - An array of all CAN signals.
 * signalCount - The size of the CAN signals array.
 * value - The value to write.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
void numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, double value, uint8_t destination[], bool* send);

/* Public: Interpret the JSON value as a double, then do the same as
 * numberWriter(CanSignal*, CanSignal*, int, double, bool*).
 *
 * Be aware that this function is not responsible for any memory allocated for
 * the 'value' parameter - be sure to call cJSON_Delete() on it after calling
 * this function if you created it with one of the cJSON_Create*() functions.
 */
void numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, uint8_t destination[],  bool* send);

/* Public: Convert the string value to the correct integer value for the given
 * CAN signal and write it to the signal's bitfield.
 *
 * Be aware that the behavior is undefined if there are multiple values assigned
 * to a single state. See https://github.com/openxc/vi-firmware/issues/185.
 *
 * signal - The signal associated with the value.
 * signals - An array of all CAN signals.
 * signalCount - The size of the CAN signals array.
 * value - The string object to write. The value should correspond to a signal
 *         state integer value.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
void stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, const char* value, uint8_t destination[], bool* send);

/* Public: Interpret the JSON value as a string, then do the same as
 * stateWriter(CanSignal*, CanSignal*, int, const char*, bool*).
 *
 * Be aware that this function is not responsible for any memory allocated for
 * the 'value' parameter - be sure to call cJSON_Delete() on it after calling
 * this function if you created it with one of the cJSON_Create*() functions.
 */
void stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, uint8_t destination[], bool* send);

/* Public: Write the given boolean value to the correct bitfield for the given
 * signal. This will write either a 0 or 1.
 *
 * signal - The signal associated with the value.
 * signals - An array of all CAN signals.
 * signalCount - The size of the CAN signals array.
 * value - The boolean to write.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
void booleanWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, bool value, uint8_t destination[], bool* send);

/* Public: Interpret the JSON value as a boolean, then do the same as
 * numberWriter(CanSignal*, CanSignal*, int, bool, bool*).
 *
 * Be aware that this function is not responsible for any memory allocated for
 * the 'value' parameter - be sure to call cJSON_Delete() on it after calling
 * this function if you created it with one of the cJSON_Create*() functions.
 */
void booleanWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, uint8_t destination[], bool* send);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Using the provided CanSignal and writer function, convert the cJSON value
 * into a numerical value appropriate for the CAN signal. This may include
 * converting a string state value to its numerical equivalent, for example. The
 * writer function must know how to do this conversion.
 *
 * signal - The CanSignal to send.
 * value - The value to send in the signal. This could be a boolean, number or
 *         string (i.e. a state value).
 * writer - A function to convert from the cJSON value to an encoded byte array.
 * signals - An array of all CAN signals.
 * signalCount - The size of the signals array.
 * force - true if the signals should be sent regardless of the writable status
 *         in the CAN message structure.
 *
 * Be aware that this function is not responsible for any memory allocated for
 * the 'value' parameter - be sure to call cJSON_Delete() on it after calling
 * this function if you created it with one of the cJSON_Create*() functions.
 *
 * Returns true if the message was sent successfully.
 */
bool sendSignal(CanSignal* signal, cJSON* value,
        void (*writer)(CanSignal*, CanSignal*, int, cJSON*, uint8_t[], bool*),
        CanSignal* signals, int signalCount, bool force);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Just like the above function sendSignal(), but the value of force defaults
 * to false.
 */
bool sendSignal(CanSignal* signal, cJSON* value,
        void (*writer)(CanSignal*, CanSignal*, int, cJSON*, uint8_t[], bool*),
        CanSignal* signals, int signalCount);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Just like the above function sendSignal() that accepts a writer function,
 * but uses the CanSignal's value for "writeHandler" instead.
 *
 * See above for argument descriptions.
 */
bool sendSignal(CanSignal* signal, cJSON* value, CanSignal* signals,
        int signalCount, bool force);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Just like the above function sendSignal(), but the value of force defaults
 * to false.
 */
bool sendSignal(CanSignal* signal, cJSON* value, CanSignal* signals,
        int signalCount);

/* Public: The lowest-level API available to send a CAN message. The byte order
 * of the data is swapped, but otherwise this function queues the data to write
 * out to CAN without any additional processing.
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
void processWriteQueue(CanBus* bus);

/* Public: Write a CAN message with the given data and node ID to the bus.
 *
 * Defined per-platform.
 *
 * bus - The CAN bus to send the message on.
 * request - the CanMessage requested to send.
 *
 * Returns true if the message was sent successfully.
 */
bool sendMessage(CanBus* bus, CanMessage request);

} // namespace write
} // namespace can
} // namespace openxc

#endif // _CANWRITE_H_
