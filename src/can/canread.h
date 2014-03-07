#ifndef _CANREAD_H_
#define _CANREAD_H_

#include "can/canutil.h"
#include "pipeline.h"
#include "openxc.pb.h"

namespace openxc {
namespace can {
namespace read {

typedef float (*NumericalHandler)(CanSignal*, CanSignal*, int,
                openxc::pipeline::Pipeline*, float, bool*);
typedef bool (*BooleanHandler)(CanSignal*, CanSignal*, int,
                openxc::pipeline::Pipeline*, float, bool*);
typedef const char* (*StringHandler)(CanSignal*, CanSignal*, int,
                openxc::pipeline::Pipeline*, float, bool*);

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and send the result to the pipeline.
 *
 * There are 4 versions of this function, each translating the signal into a
 * different native data type. This version omits the 'handler' argument and
 * interprets the signal as a simple floating point number.
 *
 * The version accepting a StringHandler runs the float value through the
 * handler function to convert it to a string describing a valid state for the
 * CAN signal. No error checking is performed on the handler, so if a NULL is
 * returned by the handler (e.g. because no state was found associated with the
 * float value), bad things may happen.
 *
 * pipeline - The pipeline to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data   - The raw bytes of the CAN message that contains the signal.
 * handler - (not in all versions of the function) a function that performs
 *      extra processing on the float value.
 * signals - an array of all active signals.
 * signalCount - The length of the signals array.
 */
void translateSignal(openxc::pipeline::Pipeline* pipeline, CanSignal* signal,
        uint64_t data, CanSignal* signals, int signalCount);

void translateSignal(openxc::pipeline::Pipeline* pipeline, CanSignal* signal,
        uint64_t data, NumericalHandler handler, CanSignal* signals,
        int signalCount);

void translateSignal(openxc::pipeline::Pipeline* pipeline, CanSignal* signal,
        uint64_t data, BooleanHandler handler, CanSignal* signals,
        int signalCount);
void translateSignal(openxc::pipeline::Pipeline* pipeline, CanSignal* signal,
        uint64_t data, StringHandler handler, CanSignal* signals,
        int signalCount);

/* Public: Perform no parsing or processing of the CAN message, just encapsulate
 * it in a message with "id" and "data" attributes and send it out to the
 * pipelines.
 *
 * This is useful for debugging when CAN acceptance filters are disabled. Call
 * this function every time decodeCanMessage is called and you will get a full
 * CAN message stream.
 *
 * message - the received CanMessage.
 * messages - the list of all CAN messages - if NULL or of length zero, will
 * process all messages.
 * messageCount - the length of the messages array.
 * pipeline - The pipeline to send the raw message over as an integer ID
 *      and hex data as an ASCII encoded string.
 */
void passthroughMessage(CanBus* bus, CanMessage* message,
        CanMessageDefinition* messages, int messageCount,
        openxc::pipeline::Pipeline* pipeline);

/* Public: Create a new OpenXC message and send it to the pipeline.
 *
 * There are three versions of this function, each taking a different type for
 * the 'value' field - a float, char* or bool.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The numerical value for the value field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendNumericalMessage(const char* name, float value,
        openxc::pipeline::Pipeline* pipeline);
void sendStringMessage(const char* name, const char* value,
        openxc::pipeline::Pipeline* pipeline);
void sendBooleanMessage(const char* name, bool value,
        openxc::pipeline::Pipeline* pipeline);

/* Public: Create a new OpenXC message including an 'event' and send it to the
 * pipeline.
 *
 * There are three versions of this function, each taking a different type for
 * the 'event' field - a float, char* or bool. The 'value' field is always a
 * string for evented messages.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The string value for the value field of the OpenXC message.
 * event - The boolean event for the event field of the OpenXC message.
 * pipeline - The pipeline to send on.
 */
void sendEventedBooleanMessage(const char* name, const char* value, bool event,
        openxc::pipeline::Pipeline* pipeline);
void sendEventedStringMessage(const char* name, const char* value,
        const char* event, openxc::pipeline::Pipeline* pipeline);
void sendEventedFloatMessage(const char* name, const char* value, float event,
        openxc::pipeline::Pipeline* pipeline);

/* Public: Finds and returns the corresponding string state for an integer
 *         value.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals.
 * signalCount - the length of the signals array.
 * pipeline - The pipeline for outgoing message, to optionally kick off your own
 *      output.
 * value   - The numerical value that maps to a state.
 * send    - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a string equivalent for the value if one is found in the signal's
 * possible states, otherwise NULL. If an equivalent isn't found, send is sent
 * to false.
 */
const char* stateHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        openxc::pipeline::Pipeline* pipeline, float value, bool* send);

/* Public: Coerces a numerical value to a boolean.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals
 * signalCount - The length of the signals array
 * pipeline - The pipeline for outgoing message, to optionally kick off your own
 *      output.
 * value   - The numerical value that will be converted to a boolean.
 * send    - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a boolean equivalent for value. The value of send will not be
 * changed.
 */
bool booleanHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
        openxc::pipeline::Pipeline* pipeline, float value, bool* send);

/* Public: Store the value of a signal, but flip the send flag to false.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * pipeline - The pipeline for outgoing message, to optionally kick off your own
 *      output.
 * value   - The numerical value that will be converted to a boolean.
 * send    - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns the original value unmodified and doesn't modify send.
 */
float passthroughHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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

/* Public: Serialize the message to a bytestream (conforming to the OpenXC
 * standard and the currently selected payload format) and send it out to the
 * pipeline.
 *
 * This will accept both raw and translated typed messages.
 *
 * TODO this doesn't belong here since it now can send command responses, it
 * belongs up a level - maybe with the pipeline.
 *
 * message - A message structure containing the type and data for the message.
 * pipeline - The pipeline to send on.
 */
void sendVehicleMessage(openxc_VehicleMessage* message,
        openxc::pipeline::Pipeline* pipeline);

} // namespace read
} // namespace can
} // namespace openxc

#endif // _CANREAD_H_
