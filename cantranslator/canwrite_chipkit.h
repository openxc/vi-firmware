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

#endif // _CANWRITE_CHIPKIT_H_
