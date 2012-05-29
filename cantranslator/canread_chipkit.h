#ifndef _CANREAD_CHIPKIT_H_
#define _CANREAD_CHIPKIT_H_

#include "canutil.h"
#include "usbutil.h"

/* Public: Parses a CAN signal from a CAN message, applies required
 *         transforations and sends the result over USB.
 *
 * usbDevice - the USB device to send the final formatted message on.
 * signal - the details of the signal to decode and forward.
 * data   - the raw bytes of the CAN message that contains the signal.
 */
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal, uint8_t* data,
        CanSignal* signals, int signalCount);

/* Public: Parses a CAN signal from a CAN message, applies required
 *         transforations and also runs the final float value through the
 *         handler function before sending the result out over USB.
 *
 * usbDevice - the USB device to send the final formatted message on.
 * signal - the details of the signal to decode and forward.
 * data - the raw bytes of the CAN message that contains the signal.
 * handler - a function pointer that performs extra processing on the
 *           float value.
 * signals - an array of all active signals.
 * signalCount - the length of the signals array
 */
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

// TODO document these
void sendNumericalMessage(char* name, float value, CanUsbDevice* usbDevice);
void sendStringMessage(char* name, char* value, CanUsbDevice* usbDevice);
void sendBooleanMessage(char* name, bool value, CanUsbDevice* usbDevice);
void sendEventedBooleanMessage(char* name, char* value, bool event,
        CanUsbDevice* usbDevice);

#endif // _CANREAD_CHIPKIT_H_
