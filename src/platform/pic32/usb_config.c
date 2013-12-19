#include "usb_config.h"
#include "USB/usb.h"

ROM USB_DEVICE_DESCRIPTOR device_dsc=
{
    sizeof(USB_DEVICE_DESCRIPTOR),
    USB_DESCRIPTOR_DEVICE,
    USB_VERSION,
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    CONTROL_ENDPOINT_SIZE,  // Max packet size for EP0, see usb_config.h
    VENDOR_ID,                 // Vendor ID
    PRODUCT_ID,                 // Product ID
    DEVICE_VERSION,
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x00,                   // Device uart number string index
    NUM_CONFIGURATIONS
};

/* Configuration 1 Descriptor */
ROM BYTE configDescriptor1[]={
    sizeof(USB_CONFIGURATION_DESCRIPTOR),
    USB_DESCRIPTOR_CONFIGURATION,                // CONFIGURATION descriptor type
    0x27,0x00,            // Total length of data for this cfg
    INTERFACE_COUNT,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT | _SELF,               // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)

    /* Interface Descriptor */
    sizeof(USB_INTERFACE_DESCRIPTOR),
    USB_DESCRIPTOR_INTERFACE,
    0,                      // Interface Number
    0,                      // Alternate Setting Number
    ENDPOINT_COUNT,         // Number of endpoints in this intf
    0xFF,                   // Class code
    0xFF,                   // Subclass code
    0xFF,                   // Protocol code
    0,                      // Interface string index

    /* Endpoint Descriptor */
    sizeof(USB_ENDPOINT_DESCRIPTOR),
    USB_DESCRIPTOR_ENDPOINT,    // Endpoint Descriptor
    (ENDPOINT_DIR_OUT | OUT_ENDPOINT_NUMBER), // EndpointAddress
    _BULK,                       // Attributes
    DATA_ENDPOINT_SIZE,0x00,
    1,                         // Interval, unused by bulk endpoint

    sizeof(USB_ENDPOINT_DESCRIPTOR),
    USB_DESCRIPTOR_ENDPOINT, //Endpoint Descriptor
    (ENDPOINT_DIR_IN | IN_ENDPOINT_NUMBER),                   // EndpointAddress
    _BULK,                       // Attributes
    DATA_ENDPOINT_SIZE,0x00,
    1,                         // Interval, unused by bulk endpoint

    sizeof(USB_ENDPOINT_DESCRIPTOR),
    USB_DESCRIPTOR_ENDPOINT, //Endpoint Descriptor
    (ENDPOINT_DIR_IN | LOG_ENDPOINT_NUMBER),                   // EndpointAddress
    _BULK,                       // Attributes
    DATA_ENDPOINT_SIZE,0x00,
    1,                         // Interval, unused by bulk endpoint
};


//Language code string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[1];}sd000={
sizeof(sd000),USB_DESCRIPTOR_STRING,{0x0409}};

//Manufacturer string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[18];}sd001={
sizeof(sd001),USB_DESCRIPTOR_STRING,
{'F','o','r','d',' ','M','o','t','o','r', ' ', 'C','o','m','p','a','n','y'}};

//Product string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[24];}sd002={
sizeof(sd002),USB_DESCRIPTOR_STRING,
{'O','p','e','n','X','C',' ','V','e','h','i','c','l','e',' ',
    'I','n','t','e','r','f','a','c','e'}};

//Array of configuration descriptors
ROM BYTE *ROM USB_CD_Ptr[]=
{
    (ROM BYTE *ROM)&configDescriptor1
};
//Array of string descriptors
ROM BYTE *ROM USB_SD_Ptr[]=
{
    (ROM BYTE *ROM)&sd000,
    (ROM BYTE *ROM)&sd001,
    (ROM BYTE *ROM)&sd002
};
