#ifndef __INTERFACE_USB_H__
#define __INTERFACE_USB_H__

#include <string.h>
#include <stdint.h>

#ifdef __PIC32__
#include "chipKITUSBDevice.h"
#endif // __PIC32__

#include "interface.h"
#include "usb_config.h"
#include "util/bytebuffer.h"

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
 * queue - A queue of bytes from or for IN or OUT requests, depending on the
 *      direction.
 */
typedef struct {
    uint8_t address;
    uint8_t size;
    UsbEndpointDirection direction;
    QUEUE_TYPE(uint8_t) queue;
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
 * descriptor - A general descriptor for this interface.
 * endpoints - An array of addresses for the endpoints to use.
 * configured - A flag that indicates if the USB interface has been configured
 *      by a host. Once true, this will not be set to false until the board is
 *      reset.
 *
 * device - The UsbDevice attached to the host - only used on PIC32.
 */
typedef struct {
    InterfaceDescriptor descriptor;
    // TODO what if we had two UsbEndpoint types, one for in and one for out?
    // how would we index into the array?
    UsbEndpoint endpoints[ENDPOINT_COUNT];
    bool configured;
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
void read(UsbDevice* device, openxc::util::bytebuffer::IncomingMessageCallback callback);

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
void read(UsbDevice* device, UsbEndpoint* endpoint,
        openxc::util::bytebuffer::IncomingMessageCallback callback);

/* Public: Send any bytes in the outgoing data queue over the IN endpoint to the
 * host.
 *
 * This function may or may not be blocking - it's implementation dependent.
 */
void processSendQueue(UsbDevice* device);

/* Public: Disconnect from host and turn off the USB peripheral
 *           (minimal power draw).
 */
void deinitialize(UsbDevice*);

/* Public: Perform platform-agnostic USB de-initialization.
 */
void deinitializeCommon(UsbDevice*);

size_t handleIncomingMessage(uint8_t payload[], size_t length);

/* Public: Check the connection status of a USB device.
 *
 * Returns true if a USB host is connected.
 */
bool connected(UsbDevice* device);

} // namespace usb
} // namespace interface
} // namespace openxc

#endif // __INTERFACE_USB_H__
