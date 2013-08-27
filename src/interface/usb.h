#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#ifdef __PIC32__
#include "chipKITUSBDevice.h"
#endif // __PIC32__

#include <string.h>
#include <stdint.h>
#include "util/bytebuffer.h"

#define USB_BUFFER_SIZE 64
#define USB_SEND_BUFFER_SIZE 512
#define MAX_USB_PACKET_SIZE_BYTES USB_BUFFER_SIZE

namespace openxc {
namespace interface {
namespace usb {

/* Public: a container for a VI USB device and associated metadata.
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
    QUEUE_TYPE(uint8_t) sendQueue;
    QUEUE_TYPE(uint8_t) receiveQueue;
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
void initializeCommon(UsbDevice*);

/* Public: Initializes the USB controller as a full-speed device with the
 * configuration specified in usb_descriptors.c. Must be called before
 * any other USB fuctions are used.
 */
void initialize(UsbDevice*);

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
void read(UsbDevice* device, bool (*callback)(uint8_t*));

/* Public: Send any bytes in the outgoing data queue over the IN endpoint to the
 * host.
 *
 * This function may or may not be blocking - it's implementation dependent.
 */
void processSendQueue(UsbDevice* device);

/* Public: Send a USB control message on EP0 (the endponit used only for control
 * transfers).
 *
 * data - An array of up bytes up to the total size of the endpoint (64 bytes
 *      for USB 2.0)
 * length - The length of the data array.
 */
void sendControlMessage(uint8_t* data, uint8_t length);

/* Public: Disconnect from host and turn off the USB peripheral
 *           (minimal power draw).
 */
void deinitialize(UsbDevice*);

} // namespace usb
} // namespace interface
} // namespace openxc

#endif // _USBUTIL_H_
