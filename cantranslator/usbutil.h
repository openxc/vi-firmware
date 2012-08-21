#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#include "usbutil.h"
#include <string.h>
#include <stdint.h>
// #include "serialutil.h"

extern "C" {
#include "serial_fifo.h"
#include "usbapi.h"
}

#define MAX_USB_PACKET_SIZE 64

// Don't try to send a message larger than this
#define PACKET_BUFFER_SIZE MAX_USB_PACKET_SIZE * 4

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

/* Configuration Attributes */
#define _DEFAULT    (0x01<<7)       //Default Value (Bit 7 is set)
#define _SELF       (0x01<<6)       //Self-powered (Supports if set)
#define _RWU        (0x01<<5)       //Remote Wakeup (Supports if set)
#define _HNP	    (0x01 << 1)     //HNP (Supports if set)
#define _SRP	  	(0x01)		    //SRP (Supports if set)

/* Endpoint Transfer Type */
#define _CTRL       0x00            //Control Transfer
#define _ISO        0x01            //Isochronous Transfer
#define _BULK       0x02            //Bulk Transfer

#define _EP01_OUT   0x01
#define _EP01_IN    0x81

static const U8 USB_DESCRIPTORS[] = {

    // Device descriptor
	0x12,              		// size of the descriptor in bytes
	DESC_DEVICE,
	LE_WORD(0x0110),		// bcdUSB
	0x00,              		// bDeviceClass
	0x00,              		// bDeviceSubClass
	0x00,              		// bDeviceProtocol
	MAX_USB_PACKET_SIZE,  		// bMaxPacketSize
	LE_WORD(0xFFFF),		// idVendor
	LE_WORD(0x0001),		// idProduct
	LE_WORD(0x0100),		// bcdDevice
	0x01,              		// iManufacturer
	0x02,              		// iProduct
	0x03,              		// iSerialNumber
	0x01,              		// bNumConfigurations

    // Configuration 1 Descriptor
    0x09,                   // Size of this descriptor in bytes
    DESC_CONFIGURATION,                // CONFIGURATION descriptor type
	LE_WORD(0x22),  		// wTotalLength
	0x01,  					// bNumInterfaces
	0x01,  					// bConfigurationValue
	0x00,  					// iConfiguration
	0x80,  					// bmAttributes
	0x32,  					// bMaxPower

    // Interface Descriptor
	0x09,
	DESC_INTERFACE,
	0x00,  		 			// bInterfaceNumber
	0x00,   				// bAlternateSetting
	0x02,   				// bNumEndPoints
	0x03,   				// bInterfaceClass = HID
	0x00,   				// bInterfaceSubClass
	0x00,   				// bInterfaceProtocol
	0x00,   				// iInterface

    // Endpoint descriptor
	0x07,
	DESC_ENDPOINT,
	_EP01_OUT,				// bEndpointAddress
	_BULK,   				// bmAttributes = INT
	LE_WORD(MAX_USB_PACKET_SIZE),// wMaxPacketSize
	1,						// bInterval

	0x07,
	DESC_ENDPOINT,
	_EP01_IN,				// bEndpointAddress
	_BULK,   				// bmAttributes = INT
	LE_WORD(MAX_USB_PACKET_SIZE), // wMaxPacketSize
	1,						// bInterval

    // language code string descriptors
	0x04,
	DESC_STRING,
	LE_WORD(0x0409),

	// manufacturer string
	0x0E,
	DESC_STRING,
    'F','o','r','d',' ','M','o','t','o','r', 'C','o','m','p','a','n','y',

	// product string
	0x12,
	DESC_STRING,
    'O','p','e','n','X','C',' ','C','A','N',' ',
    'T','r','a','n','s', 'l','a','t','o','r',

	// serial number string
	0x12,
	DESC_STRING,
	'D', 0, 'E', 0, 'A', 0, 'D', 0, 'C', 0, '0', 0, 'D', 0, 'E', 0,

	// terminator
	0
};

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
    int endpoint;
    int endpointSize;
    // SerialDevice* serial;
    bool configured;
    // device to host
    char sendBuffer[MAX_USB_PACKET_SIZE];
    // host to device
    char receiveBuffer[MAX_USB_PACKET_SIZE];
    fifo_t transmitQueue;
    // buffer messages up to 4x 1 USB packet in size waiting for valid JSON
    char packetBuffer[PACKET_BUFFER_SIZE];
    int packetBufferIndex;
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
void* armForRead(CanUsbDevice* usbDevice, char* buffer);

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
void* readFromHost(CanUsbDevice* usbDevice, void* handle,
        bool (*callback)(char*));

/* Internal: Handle asynchronous events from the USB controller.
 */
// static bool usbCallback(void* event, void *pdata, int size);

extern CanUsbDevice USB_DEVICE;

#endif // _USBUTIL_H_
