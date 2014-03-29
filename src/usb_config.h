#ifndef _USB_CONFIG_H_
#define _USB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ENDPOINT_DIR_OUT 0x00
#define ENDPOINT_DIR_IN 0x80

// TODO There's no reason in the USB spec why these endpoints have to have
// different numbers. Previously, we used EP1 in both the IN and OUT directions
// for all data. There is a bug in the nxpUSBlib (referenced here:
// http://lpcware.com/content/forum/problem-and-out-endpoints) where both
// directions of an endpoint share a single buffer, so this is broken.
#define IN_ENDPOINT_NUMBER 2
#define OUT_ENDPOINT_NUMBER 5
#define LOG_ENDPOINT_NUMBER 11
#define INTERFACE_COUNT 1
#define ENDPOINT_COUNT 3
// Take note - this is not the *number of endpoints* but the highest endpoint
// number used, e.g. we have 2 endpoints but one is 5 and the other is 11 - this
// must be 11!
#define MAX_ENDPOINT_NUMBER 11
#define CONTROL_ENDPOINT_SIZE 64
#define DATA_ENDPOINT_SIZE 64

#define IN_ENDPOINT_INDEX 0
#define OUT_ENDPOINT_INDEX 1
#define LOG_ENDPOINT_INDEX 2

// Ford Motor Company USB Vendor ID
#define VENDOR_ID 0x1bc4

#define PRODUCT_ID DEFAULT_USB_PRODUCT_ID

#define NUM_CONFIGURATIONS 0x01
#define USB_VERSION 0x0200
#define DEVICE_VERSION 0x100

#ifdef __LPC17XX__

#include "USB/USB.h"

typedef struct {
    USB_Descriptor_Configuration_Header_t    Config;
    USB_Descriptor_Interface_t               Interface;
    USB_Descriptor_Endpoint_t                InEndpoint;
    USB_Descriptor_Endpoint_t                OutEndpoint;
    USB_Descriptor_Endpoint_t                LogEndpoint;
} USB_Descriptor_Configuration_t;

/** This function is called by the library when in device mode, and must be
 * overridden (see library "USB Descriptors" documentation) by the application
 * code so that the address and size of a requested descriptor can be given to
 * the USB library. When the device receives a Get Descriptor request on the
 * control endpoint, this function is called so that the descriptor details can
 * be passed back and the appropriate descriptor sent back to the USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint8_t wIndex,
        const void** const DescriptorAddress)
        ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif // __LPC17XX__

#ifdef __PIC32__

#define USB_MAX_NUM_INT INTERFACE_COUNT
#define USB_MAX_EP_NUMBER MAX_ENDPOINT_NUMBER
#define USB_EP0_BUFF_SIZE CONTROL_ENDPOINT_SIZE

//Device descriptor - if these two definitions are not defined then
//  a ROM USB_DEVICE_DESCRIPTOR variable by the exact name of device_dsc
//  must exist.
#define USB_USER_DEVICE_DESCRIPTOR &device_dsc
#define USB_USER_DEVICE_DESCRIPTOR_INCLUDE extern ROM USB_DEVICE_DESCRIPTOR device_dsc

//Configuration descriptors - if these two definitions do not exist then
//  a ROM BYTE *ROM variable named exactly USB_CD_Ptr[] must exist.
#define USB_USER_CONFIG_DESCRIPTOR USB_CD_Ptr
#define USB_USER_CONFIG_DESCRIPTOR_INCLUDE extern ROM BYTE *ROM USB_CD_Ptr[]

#define USB_PING_PONG_MODE USB_PING_PONG__FULL_PING_PONG

#define USB_INTERRUPT

#define USB_PULLUP_OPTION USB_PULLUP_ENABLE
#define USB_TRANSCEIVER_OPTION USB_INTERNAL_TRANSCEIVER
#define USB_SPEED_OPTION USB_FULL_SPEED
#define USB_ENABLE_STATUS_STAGE_TIMEOUTS

#define USB_STATUS_STAGE_TIMEOUT     (BYTE)45 // Timeout in ms

#define USB_SUPPORT_DEVICE

#define USB_NUM_STRING_DESCRIPTORS 3

#define USB_ENABLE_ALL_HANDLERS

// Generic device class
#define USB_USE_GEN

#define USBGEN_EP_NUM            1

#endif // __PIC32__

#ifdef __cplusplus
}
#endif

#endif // _USB_CONFIG_H_
