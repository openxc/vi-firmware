#ifndef _CANWRITE_H_
#define _CANWRITE_H_

#include "canutil.h"

typedef bool (*WriteHandler)(char* name, cJSON* value, CanSignal* signals,
        int signalCount);

struct CanCommand {
    char* genericName;
    WriteHandler handler;
};

/* Public: Encode and store value in a bit field for the given signal.
 *
 * The value is converted to engineering units (i.e. any offset or factor used
 * by the signal are reversed) before being set into a 64-bit chunk at the
 * location determined by the bit field in the signal. All other bit fields in
 * the returend value will be 0, which may or may not interfere with other
 * recipients of the resulting CAN message.
 *
 * signal - The signal associated with the value
 * value - The numerical value to encode. String-ified state values need to be
 *      converted back to their integer equivalents before calling this
 *      function.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
uint64_t encodeCanSignal(CanSignal* signal, float value);

/* Public: Interpret the JSON value as a number and write it to the correct
 * bitfield for the given signal.
 *
 * signal - The signal associated with the value.
 * signals - An array of all CAN signals.
 * signalCount - The size of the CAN signals array.
 * value - The JSON object to write. The value will be interpreted as a double.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
uint64_t numberWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send);

/* Public: Interpret the JSON value as a string, convert it to the correct
 * integer value for the given CAN signal and write it to the signal's bitfield.
 *
 * signal - The signal associated with the value.
 * signals - An array of all CAN signals.
 * signalCount - The size of the CAN signals array.
 * value - The JSON object to write. The value will be interpreted as a string
 *      that corresponds to a signal state.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
uint64_t stateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send);

/* Public: Interpret the JSON value as a boolean and write it to the correct
 * bitfield for the given signal. This will write either a 0 or 1.
 *
 * signal - The signal associated with the value.
 * signals - An array of all CAN signals.
 * signalCount - The size of the CAN signals array.
 * value - The JSON object to write. The value will be interpreted as a integer
 *      that represents a boolean.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a 64-bit data block with the bit field for the signal set to the
 * encoded value.
 */
uint64_t booleanWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send);

CanCommand* lookupCommand(char* name, CanCommand* commands, int signalCount);

#endif // _CANWRITE_H_
