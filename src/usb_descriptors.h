#ifndef __USB_DESCRIPTORS_H__
#define __USB_DESCRIPTORS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "USB/USB.h"

#define DATA_ENDPOINT_NUMBER 1
#define CONTROL_ENDPOINT_SIZE 64
#define DATA_ENDPOINT_SIZE 64

typedef struct {
    USB_Descriptor_Configuration_Header_t    Config;
    USB_Descriptor_Interface_t               Interface;
    USB_Descriptor_Endpoint_t                InEndpoint;
    USB_Descriptor_Endpoint_t                OutEndpoint;
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

#ifdef __cplusplus
}
#endif

#endif // __USB_DESCRIPTORS_H__
