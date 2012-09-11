#ifndef __USB_CONFIG_H__
#define __USB_CONFIG_H__

#define USB_EP0_BUFF_SIZE       64
#define USB_MAX_NUM_INT         1
#define USB_MAX_EP_NUMBER       1

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

#define USBGEN_EP_SIZE          64
#define USBGEN_EP_NUM            1

#endif // __USB_CONFIG_H__
