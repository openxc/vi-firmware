#ifndef _CANWRITE_H_
#define _CANWRITE_H_

#include "canutil.h"

/* Public: Encode and store value in a bit field for the given signal.
 *
 * The value is converted to engineering units (i.e. any offset or factor used
 * by the signal are reversed) before being set into a 64-bit chunk at the
 * location determined by the bit field in the signal. All other bit fields in
 * the returend value will be 0, which may or may not interfere with other
 * recipients of the resulting CAN message.
 *
 * signal - The signal associated with the value
 * value  - The numerical value to encode. String-ified state values need to be
 *      converted back to their integer equivalents before calling this
 *      function.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
uint64_t encodeCanSignal(CanSignal* signal, float value);

uint64_t numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send);

uint64_t stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send);

uint64_t booleanWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send);

#endif // _CANWRITE_H_
