#ifndef _CANREAD_H_
#define _CANREAD_H_

#include "canutil.h"
#include "usbutil.h"

#define NAME_FIELD_NAME "name"
#define VALUE_FIELD_NAME "value"
#define EVENT_FIELD_NAME "event"

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and send the result over USB.
 *
 * usbDevice - The USB device to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data   - The raw bytes of the CAN message that contains the signal.
 */
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal, uint8_t* data,
        CanSignal* signals, int signalCount);

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and also run the final float value through the handler
 * function before sending the result out over USB.
 *
 * usbDevice - The USB device to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data - The raw bytes of the CAN message that contains the signal.
 * handler - a function that performs extra processing on the float value.
 * signals - an array of all active signals.
 * signalCount - The length of the signals array.
 */
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and (expecting the float value to be 0 or 1) convert it to a
 * boolean.
 *
 * usbDevice - The USB device to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data - The raw bytes of the CAN message that contains the signal.
 * handler - a function that converts the float value to a boolean.
 * signals - an array of all active signals.
 * signalCount - The length of the signals array
 */
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

/* Public: Parse a CAN signal from a CAN message, apply the required
 * transforations and also runs the float value through the handler function to
 * convert it to a string describing a valid state for the CAN signal. No error
 * checking is performed on the handler, so if a NULL is returned by the handler
 * (e.g. because no state was found associated with the float value), bad things
 * may happen.
 *
 * TODO do error checking, durr.
 *
 * usbDevice - The USB device to send the final formatted message on.
 * signal - The details of the signal to decode and forward.
 * data - The raw bytes of the CAN message that contains the signal.
 * handler - A function that returns the string state value associated with the
 *     float value.
 * signals - An array of all active signals.
 * signalCount - The length of the signals array>
 */
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

/* Public: Send the given name and value out over the default in endpoint of the
 * USB device in an OpenXC JSON message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The numerical value for the value field of the OpenXC message.
 * usbDevice - The USB device to send on.
 */
void sendNumericalMessage(char* name, float value, CanUsbDevice* usbDevice);

/* Public: Send the given name and value out over the default in endpoint of the
 * USB device in an OpenXC JSON message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The string value for the value field of the OpenXC message.
 * usbDevice - The USB device to send on.
 */
void sendStringMessage(char* name, char* value, CanUsbDevice* usbDevice);

/* Public: Send the given name and value out over the default in endpoint of the
 * USB device in an OpenXC JSON message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The boolean value for the value field of the OpenXC message.
 * usbDevice - The USB device to send on.
 */
void sendBooleanMessage(char* name, bool value, CanUsbDevice* usbDevice);

/* Public: Send the given name and value out over the default in endpoint of the
 * USB device in an OpenXC JSON message followed by a newline.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The string value for the value field of the OpenXC message.
 * event - The boolean event for the event field of the OpenXC message.
 * usbDevice - The USB device to send on.
 */
void sendEventedBooleanMessage(char* name, char* value, bool event,
        CanUsbDevice* usbDevice);

/* Public: Parse a CAN signal from a message and apply required transformation.
 *
 * signal - The details of the signal to decode and forward.
 * data   - The raw bytes of the CAN message that contains the signal.
 *
 * Returns the final, transformed value of the signal.
 */
float decodeCanSignal(CanSignal* signal, uint8_t* data);

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
char* stateHandler(CanSignal* signal, CanSignal* signals, int signalCount,
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

#endif // _CANREAD_H_
