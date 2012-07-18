#ifndef _CANWRITE_CHIPKIT_H_
#define _CANWRITE_CHIPKIT_H_

#include "canutil.h"
#include "cJSON.h"

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Using the provided CanSignal and writer function, convert the cJSON value
 * into a numerical value appropriate for the CAN signal. This may include
 * converting a string state value to its numerical equivalent, for example. The
 * writer function must know how to do this conversion (and return a fully
 * filled out uint64_t).
 *
 * signal - The CanSignal to send.
 * value - The value to send in the signal. This could be a boolean, number or
 *         string (i.e. a state value).
 * writer - A function to convert from the cJSON value to an encoded uint64_t.
 * signals - An array of all CAN signals.
 * signalCount - The size of the signals array.
 *
 * Returns true if the message was sent successfully.
 */
bool sendCanSignal(CanSignal* signal, cJSON* value,
        uint64_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*),
        CanSignal* signals, int signalCount);

/* Public: Write a CAN signal with the given value to the bus.
 *
 * Just like the above function sendCanSignal() that accepts a writer function,
 * but uses the CanSignal's value for "writeHandler" instead.
 *
 * See above for argument descriptions.
 */
bool sendCanSignal(CanSignal* signal, cJSON* value, CanSignal* signals,
        int signalCount);

/* Public: Send the given data on CAN using the signal's message ID and bus. The
 * data is assumed to be 64-bits and is used unmodified as the message's value.
 *
 * signal - the signal whose message we should write to CAN.
 * data - the data for the CAN message.
 * send - true if the message should actually be sent.
 *
 * Returns true if the message was sent on CAN.
 */
bool sendCanSignal(CanSignal* signal, uint64_t data, bool* send);

#endif // _CANWRITE_CHIPKIT_H_
