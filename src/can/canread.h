#ifndef _CANREAD_H_
#define _CANREAD_H_

#include "can/canutil.h"
#include "pipeline.h"
#include "openxc.pb.h"

namespace openxc {
namespace can {
namespace read {

/* Public: Parse a signal from a CAN message, apply any required transforations
 *      to get a human readable value and public the result to the pipeline.
 *
 * If the CanSignal has a non-NULL 'decoder' field, the raw CAN signal value
 * will be passed to the decoder before publishing.
 *
 * signal - The details of the signal to decode and forward.
 * message   - The received CAN message that should contain this signal.
 * signals - an array of all active signals.
 * signalCount - The length of the signals array.
 * pipeline -  The pipeline to send the final formatted message on if
 *      there were no errors and (if the signal has a limited frequency) it's
 *      due to send.
 *
 * The decoder returns an openxc_DynamicField, which may contain a number,
 * string or boolean.
 */
void translateSignal(const CanSignal* signal,
        CanMessage* message, const CanSignal* signals, SignalManager* signalManagers, int signalCount,
        openxc::pipeline::Pipeline* pipeline);

/* Public: Publish a CAN message to the pipeline without any parsing or
 * processing - just encapsulate it in a VehicleMessage.
 *
 * This is useful for debugging when CAN acceptance filters are disabled. Call
 * this function every time decodeCanMessage is called and you will get a full
 * CAN message stream.
 *
 * bus - the CAN bus on which this message was received.
 * message - the received message.
 * messages - the list of all CAN messages that should be published. If this
 *      argument is not NULL and the messageCount is greater than zero, the
 *      received message must be in this list or it will not be published.
 * messageCount - The length of the messages array.
 * pipeline - The pipeline to send the raw message.
 */
void passthroughMessage(CanBus* bus, CanMessage* message,
        const CanMessageDefinition* messages, int messageCount,
        openxc::pipeline::Pipeline* pipeline);

/* Public: Publish a simple vehicle message to the pipeline with a value and
 * an optional event.
 *
 * If 'event' is NULL, the published message will not have an event field.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The value for the value field of the OpenXC message.
 * event - The event for the event field of the OpenXC message.
 * pipeline - The pipeline to publish the message.
 */
void publishVehicleMessage(const char* name, openxc_DynamicField* value,
        openxc_DynamicField* event, openxc::pipeline::Pipeline* pipeline);

/* Public: Publish a simple vehicle message to the pipeline with no event.
 *
 * This is a shortcut for publishVehicleMessage(const char*, openxc_DynamicField*,
 * openxc_DynamicField*, Pipeline) where the 'event' is presumed to be NULL.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The value for the value field of the OpenXC message.
 * pipeline - The pipeline to publish the message.
 */
void publishVehicleMessage(const char* name, openxc_DynamicField* value,
                openxc::pipeline::Pipeline* pipeline);

/* Public: Create a new OpenXC message and publish it on the pipeline.
 *
 * There are three versions of this function, each taking a different type for
 * the 'value' field - a float, char* or bool. These are all shortcuts for calls
 * to publishVehicleMessage(...) so you don't have to manually build the
 * openxc_VehicleMessage.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The numerical / string / boolean value for the value field of the
 *      OpenXC message.
 * pipeline - The pipeline to publish the message.
 */
void publishNumericalMessage(const char* name, float value,
        openxc::pipeline::Pipeline* pipeline);
void publishStringMessage(const char* name, const char* value,
        openxc::pipeline::Pipeline* pipeline);
void publishStringEventedMessage(const char* name, const char* value, const char* event,
        openxc::pipeline::Pipeline* pipeline);
void publishStringEventedBooleanMessage(const char* name, const char* value, bool event,
        openxc::pipeline::Pipeline* pipeline);
void publishBooleanMessage(const char* name, bool value,
        openxc::pipeline::Pipeline* pipeline);

/* Public: Find and return the corresponding string state for a CAN signal's
 * raw integer value.
 *
 * This is an implementation of the SignalDecoder type signature, and can be
 * used directly in the CanSignal.decoder field.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals.
 * signalCount - the length of the signals array.
 * pipeline - The pipeline for outgoing message. Required by the SignalDecoder
 *      type signature but unused in this function.
 * value - The numerical value that should map to a state.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a DynamicField with a string value if a matching state is found in
 * the signal. If an equivalent isn't found, send is sent to false and the
 * return value is undefined.
 */
openxc_DynamicField stateDecoder(const CanSignal* signal, const CanSignal* signals,
        SignalManager* signalManager, SignalManager* signalManagers,
        int signalCount, openxc::pipeline::Pipeline* pipeline, float value,
        bool* send);

/* Public: Coerces a numerical value to a boolean.
 *
 * This is an implementation of the SignalDecoder type signature, and can be
 * used directly in the CanSignal.decoder field.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals
 * signalCount - The length of the signals array
 * pipeline - The pipeline for outgoing message. Required by the SignalDecoder
 *      type signature but unused in this function.
 * value - The numerical value that will be converted to a boolean.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a DynamicField with a boolean value of false if the raw signal value
 * is 0.0, otherwise true. The 'send' argument will not be modified as this
 * decoder always succeeds.
 */
openxc_DynamicField booleanDecoder(const CanSignal* signal, const CanSignal* signals,
        SignalManager* signalManager, SignalManager* signalManagers, int signalCount, 
        openxc::pipeline::Pipeline* pipeline, float value, bool* send);

/* Public: Update the metadata for a signal and the newly received value, but
 * stop anything from being published to the pipeline.
 *
 * This is an implementation of the SignalDecoder type signature, and can be
 * used directly in the CanSignal.decoder field.
 *
 * This function always flips 'send' to false.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * pipeline - The pipeline for outgoing message. Required by the SignalDecoder
 *      type signature but unused in this function.
 * value - The numerical value that will be converted to a boolean.
 * send - This output argument will always be set to false, so the caller will
 *      know not to publish this value to the pipeline.
 *
 * The return value is undefined.
 */
openxc_DynamicField ignoreDecoder(const CanSignal* signal, const CanSignal* signals,
        SignalManager* signalManager,  SignalManager* signalManagers,
        int signalCount, openxc::pipeline::Pipeline* pipeline, float value,
        bool* send);

/* Public: Wrap a raw CAN signal value in a DynamicField without modification.
 *
 * This is an implementation of the SignalDecoder type signature, and can be
 * used directly in the CanSignal.decoder field.
 *
 * signal  - The details of the signal that contains the state mapping.
 * signals - The list of all signals
 * signalCount - The length of the signals array
 * pipeline - The pipeline for outgoing message. Required by the SignalDecoder
 *      type signature but unused in this function.
 * value - The numerical value that will be wrapped in a DynamicField.
 * send - An output argument that will be set to false if the value should
 *     not be sent for any reason.
 *
 * Returns a DynamicField with the original, unmodified raw CAN signal value as
 * its numeric value. The 'send' argument will not be modified as this decoder
 * always succeeds.
 */
openxc_DynamicField noopDecoder(const CanSignal* signal, const CanSignal* signals,
         SignalManager* signalManager,  SignalManager* signalManagers,
        int signalCount, openxc::pipeline::Pipeline* pipeline, float value,
        bool* send);

/* Public: Parse the signal's bitfield from the given data and return the raw
 * value.
 *
 * signal - The signal to parse from the data.
 * data - The data to parse the signal from.
 * length - The length of the data array.
 *
 * Returns the raw value of the signal parsed as a bitfield from the given byte
 * array.
 */
float parseSignalBitfield(const CanSignal* signal, const CanMessage* message);

/* Public: Parse a signal from a CAN message and apply any required
 * transforations to get a human readable value.
 *
 * If the CanSignal has a non-NULL 'decoder' field, the raw CAN signal value
 * will be passed to the decoder before returning.
 *
 * signal - The details of the signal to decode and forward.
 * message   - The CAN message that contains the signal.
 * signals - an array of all active signals.
 * signalCount - The length of the signals array.
 * send - An output parameter that will be flipped to false if the value could
 *      not be decoded.
 *
 * The decoder returns an openxc_DynamicField, which may contain a number,
 * string or boolean. If 'send' is false, the return value is undefined.
 */
openxc_DynamicField decodeSignal(const CanSignal* signal, const CanSignal* signals,
        SignalManager* signalManager, SignalManager* signalManagers,
        int signalCount, const CanMessage* message,
        bool* send);

/* Public: Decode a transformed, human readable value from an raw CAN signal
 * already parsed from a CAN message.
 *
 * This is the same as decodeSignal(CanSignal*, CanMessage*, CanSignal*, int,
 * bool*) but you must parse the bitfield value of the signal from the CAN
 * message yourself. This is useful if you need that raw value for something
 * else.
 */
openxc_DynamicField decodeSignal(const CanSignal* signal, const CanSignal* signals,
        SignalManager* signalManager, SignalManager* signalManagers,
        int signalCount, float value, bool* send);

/* Public: Based on a signal's metadata and the most recent value received,
 * decide if it should be translated and published.
 *
 * A signal may decide not to publish if it has a limited frequency and the
 * timer hasn't expired yet, or if it's configured to only send if the value
 * changes and the it has not.
 *
 * Returns true of the value should be published.
 */
bool shouldSend(const CanSignal* signal, SignalManager* signalManager, float value);

} // namespace read
} // namespace can
} // namespace openxc

#endif // _CANREAD_H_
