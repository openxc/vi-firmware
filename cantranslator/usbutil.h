#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#include <string.h>
#include "chipKITUSBDevice.h"

#define USB_PACKET_SIZE 64
#define NUMERICAL_MESSAGE_FORMAT "{\"name\":\"%s\",\"value\":%f}\r\n"
#define BOOLEAN_MESSAGE_FORMAT "{\"name\":\"%s\",\"value\":%s}\r\n"
#define STRING_MESSAGE_FORMAT "{\"name\":\"%s\",\"value\":\"%s\"}\r\n"
#define NUMERICAL_MESSAGE_VALUE_MAX_LENGTH 6
#define BOOLEAN_MESSAGE_VALUE_MAX_LENGTH 4
#define STRING_MESSAGE_VALUE_MAX_LENGTH 24

// Don't try to send a message larger than this
#define ENDPOINT_SIZE 64


const int NUMERICAL_MESSAGE_FORMAT_LENGTH = strlen(NUMERICAL_MESSAGE_FORMAT);
const int BOOLEAN_MESSAGE_FORMAT_LENGTH = strlen(BOOLEAN_MESSAGE_FORMAT);
const int STRING_MESSAGE_FORMAT_LENGTH = strlen(STRING_MESSAGE_FORMAT);

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
    int endpointSize;
    // device to host
    char sendBuffer[ENDPOINT_SIZE];
    // host to device
    char receiveBuffer[ENDPOINT_SIZE];
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

/* Public: Arm the given endpoint for a read from the device to host.
 *
 * This also puts a NUL char in the beginning of the buffer so you don't get
 * confused that it's still a valid message.
 *
 * usbDevice - the CAN USB device to arm the endpoint on
 * buffer - the destination buffer for the next IN transfer.
 */
USB_HANDLE armForRead(CanUsbDevice* usbDevice, char* buffer);

/* Public: Pass the next IN request message to the callback, if available.
 *
 * Checks if the handle is not busy, indicating the presence of a new IN request
 * from the host. If a message is available, the callback is notified and the
 * endpoint is re-armed for the next USB transfer.
 *
 * usbDevice - the CAN USB device to arm the endpoint on
 * handle - the USB handle for IN transfers
 * callback - a function that handles USB in requests
 */
USB_HANDLE readFromHost(CanUsbDevice* usbDevice, USB_HANDLE handle,
        void (*callback)(char*));

/* Internal: Handle asynchronous events from the USB controller.
 */
static boolean usbCallback(USB_EVENT event, void *pdata, word size);

extern CanUsbDevice USB_DEVICE;
extern USB_HANDLE USB_INPUT_HANDLE;

#endif // _USBUTIL_H_
