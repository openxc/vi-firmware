#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#include <string.h>
#include "chipKITUSBDevice.h"
#include "serialutil.h"

extern "C" {
#include "queue.h"
}

// Don't try to send a message larger than this
#define ENDPOINT_SIZE 64
#define USB_PACKET_SIZE 64
#define PACKET_BUFFER_SIZE ENDPOINT_SIZE * 4

// This is a reference to the last packet read
extern volatile CTRL_TRF_SETUP SetupPkt;

/* Public: a container for a CAN translator USB device and associated metadata.
 *
 * device - The UsbDevice attached to the host.
 * endpoint - The endpoint to use to send and receive messages.
 * endpointSize - The packet size of the endpoint.
 * serial - A serial device to use in parallel to USB. TODO Yes, this is
 *      a struct for USB devices, but this is the place where this reference
 *      makes the most sense right now. Since we're actually using the
 *      hard-coded Serial1 object instead of SoftwareSerial as I had initially
 *      planned, we could actually drop this reference altogether.
 */
struct CanUsbDevice {
    USBDevice device;
    int endpoint;
    int endpointSize;
    SerialDevice* serial;
    bool configured;
    // device to host
    char sendBuffer[ENDPOINT_SIZE];
    ByteQueue sendQueue;
    // host to device
    char receiveBuffer[ENDPOINT_SIZE];
    ByteQueue receiveQueue;
    USB_HANDLE deviceToHostHandle;
    USB_HANDLE hostToDeviceHandle;
};

/* Public: Initializes the USB controller as a full-speed device with the
 * configuration specified in usb_descriptors.c. Must be called before
 * any other USB fuctions are used.
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
void armForRead(CanUsbDevice* usbDevice, char* buffer);

/* Public: Pass the next IN request message to the callback, if available.
 *
 * Checks if the input handle is not busy, indicating the presence of a new IN
 * request from the host. If a message is available, the callback is notified
 * and the endpoint is re-armed for the next USB transfer.
 *
 * usbDevice - the CAN USB device to arm the endpoint on
 * callback - a function that handles USB in requests
 */
void readFromHost(CanUsbDevice* usbDevice, bool (*callback)(uint8_t*));

/* Internal: Handle asynchronous events from the USB controller.
 */
static boolean usbCallback(USB_EVENT event, void *pdata, word size);

void processInputQueue(CanUsbDevice* usbDevice);

#endif // _USBUTIL_H_
