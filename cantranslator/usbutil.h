#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#include <string.h>
#include "chipKITUSBDevice.h"

#define USB_PACKET_SIZE 64

#define NAME_FIELD_NAME "name"
#define VALUE_FIELD_NAME "value"
#define EVENT_FIELD_NAME "event"

// Don't try to send a message larger than this
#define ENDPOINT_SIZE 128

// This is a reference to the last packet read
extern volatile CTRL_TRF_SETUP SetupPkt;

/* Public: a container for a CAN translator USB device and associated metadata.
 *
 * device - the UsbDevice attached to the host
 * endpoint - the endpoint to use to send and receive messages
 */
struct CanUsbDevice {
    USBDevice device;
    int endpoint;
};

/* Public: Initializes the USB controller as a full-speed device with the
 *         configuration specified in usb_descriptors.c. Must be called before
 *         any other USB fuctions are used.
 */
void initializeUsb(CanUsbDevice*);

/* Public: Sends a message on the bulk transfer endpoint to the host.
 *
 * usbDevice - the CAN USB device to send this message on.
 * message - a buffer containing the message to send.
 * messageSize - the length of the message.
 */
void sendMessage(CanUsbDevice* usbDevice, uint8_t* message, int messageSize);

/* Internal: Handle asynchronous events from the USB controller.
 */
static boolean usbCallback(USB_EVENT event, void *pdata, word size);

extern CanUsbDevice USB_DEVICE;
extern USB_HANDLE USB_INPUT_HANDLE;

#endif // _USBUTIL_H_
