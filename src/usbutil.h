#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#ifdef __PIC32__
#include "chipKITUSBDevice.h"
#endif // __PIC32__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdint.h>
#include "buffers.h"
#include "usb_config.h"

#define USB_BUFFER_SIZE 64
#define USB_SEND_BUFFER_SIZE 512
#define MAX_USB_PACKET_SIZE_BYTES USB_BUFFER_SIZE

/* Public: a container for a CAN translator USB device and associated metadata.
 *
 * inEndpoint - The address of the endpoint to use for IN transfers, i.e. device
 *      to host.
 * inEndpointSize - The packet size of the IN endpoint.
 * outEndpoint - The address of the endpoint to use for out transfers, i.e. host
 *      to device.
 * outEndpointSize - The packet size of the IN endpoint.
 * configured - A flag that indicates if the USB interface has been configured
 *      by a host. Once true, this will not be set to false until the board is
 *      reset.
 * sendQueue - A queue of bytes to send over the IN endpoint.
 * receiveQueue - A queue of unprocessed bytes received from the OUT endpoint.
 * device - The UsbDevice attached to the host - only used on PIC32.
 */
typedef struct {
    int inEndpoint;
    int inEndpointSize;
    int outEndpoint;
    int outEndpointSize;
    bool configured;
    ByteQueue sendQueue;
    ByteQueue receiveQueue;
    // This buffer MUST be non-local, so it doesn't get invalidated when it
    // falls off the stack
    uint8_t sendBuffer[USB_SEND_BUFFER_SIZE];
#ifdef __PIC32__
    char receiveBuffer[MAX_USB_PACKET_SIZE_BYTES];
    USBDevice device;
    USB_HANDLE deviceToHostHandle;
    USB_HANDLE hostToDeviceHandle;
#endif // __PIC32__
} UsbDevice;

/* Public: Perform platform-agnostic USB initialization.
 */
void initializeUsbCommon(UsbDevice*);

/* Public: Initializes the USB controller as a full-speed device with the
 * configuration specified in usb_descriptors.c. Must be called before
 * any other USB fuctions are used.
 */
void initializeUsb(UsbDevice*);

/* Public: Pass the next OUT request message to the callback, if available.
 *
 * Checks if the input handle is not busy, indicating the presence of a new OUT
 * request from the host. If a message is available, the callback is notified
 * and the endpoint is re-armed for the next USB transfer.
 *
 * device - The CAN USB device to arm the endpoint on.
 * callback - A function that handles USB in requests. The callback should
 *      return true if a message was properly received and parsed.
 */
void readFromHost(UsbDevice* device, bool (*callback)(uint8_t*));

/* Public: Send any bytes in the outgoing data queue over the IN endpoint to the
 * host.
 *
 * This function may or may not be blocking - it's implementation dependent.
 */
void processUsbSendQueue(UsbDevice* device);

/* Public: Send a USB control message on EP0 (the endponit used only for control
 * transfers).
 *
 * data - An array of up bytes up to the total size of the endpoint (64 bytes
 *      for USB 2.0)
 * length - The length of the data array.
 */
void sendControlMessage(uint8_t* data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif // _USBUTIL_H_
