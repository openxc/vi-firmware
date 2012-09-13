#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#include <string.h>
#include <stdint.h>
#include "queue.h"
#include "usb_config.h"

#ifdef __PIC32__
#include "chipKITUSBDevice.h"
#endif // __PIC32__


#define USB_BUFFER_SIZE 64
#define USB_SEND_BUFFER_SIZE 512
#define MAX_USB_PACKET_SIZE_BYTES USB_BUFFER_SIZE

/* Public: a container for a CAN translator USB device and associated metadata.
 *
 * device - The UsbDevice attached to the host.
 * endpoint - The endpoint to use to send and receive messages.
 * endpointSize - The packet size of the endpoint.
 */
struct UsbDevice {
#ifdef __PIC32__
    USBDevice device;
#endif // __PIC32__
    int inEndpoint;
    int inEndpointSize;
    int outEndpoint;
    int outEndpointSize;
    bool configured;
    ByteQueue sendQueue;
    // host to device
    char receiveBuffer[MAX_USB_PACKET_SIZE_BYTES];
    ByteQueue receiveQueue;
#ifdef __PIC32__
    USB_HANDLE deviceToHostHandle;
    USB_HANDLE hostToDeviceHandle;
#endif // __PIC32__
};

/* Public: Initializes the USB controller as a full-speed device with the
 * configuration specified in usb_descriptors.c. Must be called before
 * any other USB fuctions are used.
 */
void initializeUsb(UsbDevice*);

/* Public: Sends a message on the bulk transfer endpoint to the host.
 *
 * usbDevice - the CAN USB device to send this message on.
 * message - a buffer containing the message to send.
 * messageSize - the length of the message.
 */
void sendMessage(UsbDevice* usbDevice, uint8_t* message, int messageSize);

/* Public: Arm the given endpoint for a read from the device to host.
 *
 * This also puts a NUL char in the beginning of the buffer so you don't get
 * confused that it's still a valid message.
 *
 * usbDevice - the CAN USB device to arm the endpoint on
 * buffer - the destination buffer for the next IN transfer.
 */
void armForRead(UsbDevice* usbDevice, char* buffer);

/* Public: Pass the next IN request message to the callback, if available.
 *
 * Checks if the input handle is not busy, indicating the presence of a new IN
 * request from the host. If a message is available, the callback is notified
 * and the endpoint is re-armed for the next USB transfer.
 *
 * usbDevice - the CAN USB device to arm the endpoint on
 * callback - a function that handles USB in requests
 */
void readFromHost(UsbDevice* usbDevice, bool (*callback)(uint8_t*));

void processUsbSendQueue(UsbDevice* usbDevice);

void sendControlMessage(uint8_t* data, uint8_t length);

#endif // _USBUTIL_H_
