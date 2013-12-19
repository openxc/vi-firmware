#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#ifdef __PIC32__
#include "chipKITUSBDevice.h"
#endif // __PIC32__

#include <string.h>
#include <stdint.h>
#include "usb_config.h"
#include "util/bytebuffer.h"
#include "usb_config.h"

#define USB_BUFFER_SIZE 64
#define USB_SEND_BUFFER_SIZE 512
#define MAX_USB_PACKET_SIZE_BYTES USB_BUFFER_SIZE

namespace openxc {
namespace interface {
namespace usb {

typedef enum {
    USB_ENDPOINT_DIRECTION_OUT,
    USB_ENDPOINT_DIRECTION_IN
} UsbEndpointDirection;

/* Public: a container for a USB endpoint definition - one logical endpoint.
 *
 * address - the physical endpoint number.
 * size - the packet size for the endpoint, e.g. 512.
 * direction - the direction of the endpoint, IN or OUT.
 * sendQueue - A queue of bytes to send over the IN endpoint.
 * receiveQueue - A queue of unprocessed bytes received from the OUT endpoint.
 */
typedef struct {
    uint8_t address;
    uint8_t size;
    UsbEndpointDirection direction;
    QUEUE_TYPE(uint8_t) sendQueue;
    QUEUE_TYPE(uint8_t) receiveQueue;
    // This buffer MUST be non-local, so it doesn't get invalidated when it
    // falls off the stack
    uint8_t sendBuffer[USB_SEND_BUFFER_SIZE];
#ifdef __PIC32__
    char receiveBuffer[MAX_USB_PACKET_SIZE_BYTES];
    USB_HANDLE deviceToHostHandle;
    USB_HANDLE hostToDeviceHandle;
#endif // __PIC32__
} UsbEndpoint;

/* Public: a container for a VI USB device and associated metadata.
 *
 * endpoints - An array of addresses for the endpoints to use.
 * configured - A flag that indicates if the USB interface has been configured
 *      by a host. Once true, this will not be set to false until the board is
 *      reset.
 * allowRawWrites - if raw CAN messages writes are enabled for a bus and this is
 *      true, accept raw write requests from the USB interface.
 *
 * device - The UsbDevice attached to the host - only used on PIC32.
 */
typedef struct {
    // TODO what if we had two UsbEndpoint types, one for in and one for out?
    // how would we index into the array?
    UsbEndpoint endpoints[ENDPOINT_COUNT];
    bool configured;
    bool allowRawWrites;
#ifdef __PIC32__
    USBDevice device;
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

/* Public: Check for and processs OUT requests on any and all OUT endpoints.
 *
 * For each OUT endpoint, checks if the input handle is not busy, indicating the
 * presence of a new OUT request from the host. If a message is available, the
 * callback is notified and the endpoint is re-armed for the next USB transfer.
 *
 * device - The CAN USB device to arm the endpoint on.
 * callback - A function that handles USB in requests. The callback should
 *      return true if a message was properly received and parsed.
 */
void read(UsbDevice* device, bool (*callback)(uint8_t*));

/* Public: Pass the next OUT request message to the callback, if available.
 *
 * Checks if the input handle is not busy, indicating the presence of a new OUT
 * request from the host. If a message is available, the callback is notified
 * and the endpoint is re-armed for the next USB transfer.
 *
 * device - The USB device to arm the endpoint on.
 * endpoint - The endpoint to read from.
 * callback - A function that handles USB in requests. The callback should
 *      return true if a message was properly received and parsed.
 */
void read(UsbDevice* device, UsbEndpoint* endpoint, bool (*callback)(uint8_t*));

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

/* Public: Perform platform-agnostic USB de-initialization.
 */
void deinitializeCommon(UsbDevice*);

} // namespace usb
} // namespace interface
} // namespace openxc

#endif // _USBUTIL_H_
