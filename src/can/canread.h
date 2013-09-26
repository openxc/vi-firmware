#ifndef _CANREAD_H_
#define _CANREAD_H_

#include "can/canutil.h"
#include "pipeline.h"

using openxc::pipeline::Pipeline;

namespace openxc {
namespace can {
namespace read {

extern const char BUS_FIELD_NAME[];
extern const char ID_FIELD_NAME[];
extern const char DATA_FIELD_NAME[];
extern const char NAME_FIELD_NAME[];
extern const char VALUE_FIELD_NAME[];
extern const char EVENT_FIELD_NAME[];

/* Public: Perform no parsing or processing of the CAN message, just encapsulate
 * it in a JSON message with "id" and "data" attributes and send it out to the
 * pipelines.
 *
 * This is useful for debugging when CAN acceptance filters are disabled. Call
 * this function every time decodeCanMessage is called and you will get a full
 * CAN message stream.
 *
 * id - the ID of the CAN message.
 * data - 64 bits of data from the message.
 * messages - the list of all CAN messages - if NULL or of length zero, will
 * process all messages.
 * messageCount - the length of the messages array.
 * pipeline - The pipeline to send the raw message over as an integer ID
 *      and hex data as an ASCII encoded string.
 */
void passthroughMessage(CanBus* bus, uint32_t id, uint64_t data,
        CanMessageDefinition* messages, int messageCount, Pipeline* pipeline);

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and send the result to the pipeline;
 *
 * pipeline - The pipeline to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data   - The raw bytes of the CAN message that contains the signal.
 */
void translateSignal(Pipeline* pipeline, CanSignal* signal, uint64_t data,
        CanSignal* signals, int signalCount);

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and also run the final float value through the handler
 * function before sending the result to the pipeline.
 *
 * pipeline - The pipeline to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data - The raw bytes of the CAN message that contains the signal.
 * handler - a function that performs extra processing on the float value.
 * signals - an array of all active signals.
 * signalCount - The length of the signals array.
 */
void translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and (expecting the float value to be 0 or 1) convert it to a
 * boolean.
 *
 * pipeline - The pipeline to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data - The raw bytes of the CAN message that contains the signal.
 * handler - a function that converts the float value to a boolean.
 * signals - an array of all active signals.
 * signalCount - The length of the signals array
 */
void translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and also runs the float value through the handler function to
 * convert it to a string describing a valid state for the CAN signal. No error
 * checking is performed on the handler, so if a NULL is returned by the handler
 * (e.g. because no state was found associated with the float value), bad things
 * may happen.
 *
 * pipeline - The pipeline to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data - The raw bytes of the CAN message that contains the signal.
 * handler - A function that returns the string state value associated with the
 *     float value.
 * signals - An array of all active signals.
 * signalCount - The length of the signals array>
 */
void translateSignal(Pipeline* pipeline, CanSignal* signal,
        uint64_t data,
        const char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

/* Public: Send the given name and value out to the pipeline in an OpenXC JSON
 * message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The numerical value for the value field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendNumericalMessage(const char* name, float value, Pipeline* pipeline);

/* Public: Send the given name and value out to the pipeline in an OpenXC JSON
 * message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The string value for the value field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendStringMessage(const char* name, const char* value, Pipeline* pipeline);

/* Public: Send the given name and value out to the pipeline in an OpenXC JSON
 * message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The boolean value for the value field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendBooleanMessage(const char* name, bool value, Pipeline* pipeline);

/* Public: Send the given name, value and event out to the pipeline in an OpenXC
 * JSON message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The string value for the value field of the OpenXC message.
 * event - The boolean event for the event field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendEventedBooleanMessage(const char* name, const char* value, bool event,
        Pipeline* pipeline);

/* Public: Send the given name, value and event out to the pipeline in an OpenXC
 * JSON message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The string value for the value field of the OpenXC message.
 * event - The string event for the event field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendEventedStringMessage(const char* name, const char* value,
        const char* event, Pipeline* pipeline);

/* Public: Send the given name, value and event out to the pipeline in an OpenXC
 * JSON message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The string value for the value field of the OpenXC message.
 * event - The float event for the event field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendEventedFloatMessage(const char* name, const char* value, float event,
        Pipeline* pipeline);

/* Public: Parse a CAN signal from a message and apply required transformation.
 *
 * signal - The details of the signal to decode and forward.
 * data   - The raw bytes of the CAN message that contains the signal, assumed
 *      to be in big-endian byte order from CAN.
 *
 * Returns the final, transformed value of the signal.
 */
float decodeSignal(CanSignal* signal, uint64_t data);

/* Public: Finds and returns the corresponding string state for an integer
 *         value.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals.
 * signalCount - the length of the signals array.
 * value   - The numerical value that maps to a state.
 * send    - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a string equivalent for the value if one is found in the signal's
 * possible states, otherwise NULL. If an equivalent isn't found, send is sent
 * to false.
 */
const char* stateHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Public: Coerces a numerical value to a boolean.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals
 * signalCount - The length of the signals array
 * value   - The numerical value that will be converted to a boolean.
 * send    - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a boolean equivalent for value. The value of send will not be
 * changed.
 */
bool booleanHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Public: Store the value of a signal, but flip the send flag to false.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value   - The numerical value that will be converted to a boolean.
 * send    - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns the original value unmodified and sets send to false;
 */
float ignoreHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Public: Store the value of a signal, but flip the send flag to false.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value   - The numerical value that will be converted to a boolean.
 * send    - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns the original value unmodified and doesn't modify send.
 */
float passthroughHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Public: Determine if the received signal should be sent out and update
 * signal metadata.
 *
 * signal - The signal to look for in the CAN message data.
 * data - The data of the CAN message.
 * send - Will be flipped to false if the signal should not be sent (e.g. the
 *      signal is on a limited send frequency and the timer is not up yet).
 *
 * Returns the float value of the signal decoded from the data.
 */
float preTranslate(CanSignal* signal, uint64_t data, bool* send);

/* Public: Update signal metadata after translating and sending.
 *
 * We keep track of the last value of each CAN signal (in its raw float form),
 * but we can't update the value until after all translation has happened,
 * in case a custom handler needs to use the value.
 */
void postTranslate(CanSignal* signal, float value);

} // namespace read
} // namespace can
} // namespace openxc

#endif // _CANREAD_H_
