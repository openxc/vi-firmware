#ifdef __PIC32__

/** INCLUDES *******************************************************/
#include "./USB/usb.h"

/* Device Descriptor */
ROM USB_DEVICE_DESCRIPTOR device_dsc=
{
    0x12,                   // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,  // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,          // Max packet size for EP0, see usb_config.h
    0x04D8,                 // Vendor ID: 0x04D8 is Microchip's Vendor ID
    0x0053,                 // Product ID: 0x0053
    0x0000,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x03,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

/* Configuration 1 Descriptor */
ROM BYTE configDescriptor1[]={
    /* Configuration Descriptor */
    0x09,//sizeof(USB_CFG_DSC),    // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,                // CONFIGURATION descriptor type
    0x20,0x00,            // Total length of data for this cfg
    1,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT | _SELF,               // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)

    /* Interface Descriptor */
    0x09,//sizeof(USB_INTF_DSC),   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,               // INTERFACE descriptor type
    0,                      // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    0xFF,                   // Class code
    0xFF,                   // Subclass code
    0xFF,                   // Protocol code
    0,                      // Interface string index

    /* Endpoint Descriptor */
    0x07,                       /*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    _EP01_OUT,                  //EndpointAddress
    _BULK,                       //Attributes
    USBGEN_EP_SIZE,0x00,        //size
    1,                         //Interval

    0x07,                       /*sizeof(USB_EP_DSC)*/
    USB_DESCRIPTOR_ENDPOINT,    //Endpoint Descriptor
    _EP01_IN,                   //EndpointAddress
    _BULK,                       //Attributes
    USBGEN_EP_SIZE,0x00,        //size
    1                          //Interval
};


//Language code string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[1];}sd000={
sizeof(sd000),USB_DESCRIPTOR_STRING,{0x0409}};

//Manufacturer string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[25];}sd001={
sizeof(sd001),USB_DESCRIPTOR_STRING,
{'F','o','r','d',' ','M','o','t','o','r', 'C','o','m','p','a','n','y'}};

//Product string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[31];}sd002={
sizeof(sd002),USB_DESCRIPTOR_STRING,
{'O','p','e','n','X','C',' ','C','A','N',' ',
    'T','r','a','n','s', 'l','a','t','o','r'}};

ROM struct{BYTE bLength;BYTE bDscType;WORD string[4];}sd003={
sizeof(sd003),USB_DESCRIPTOR_STRING,
{'0','1','2','3'}};

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
    (ROM BYTE *ROM)&sd002,
    (ROM BYTE *ROM)&sd003
};

#endif // __PIC32__
