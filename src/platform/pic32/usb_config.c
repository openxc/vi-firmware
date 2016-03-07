#include "usb_config.h"
#include "USB/usb.h"
#include "platform_profile.h"

#ifdef FS_SUPPORT
#include "USB/usb_function_msd.h"
#endif

ROM USB_DEVICE_DESCRIPTOR device_dsc_gen=
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
ROM BYTE configDescriptor_gen[]={
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

#ifdef FS_SUPPORT    
ROM USB_DEVICE_DESCRIPTOR device_dsc_msd=
{
    0x12,    // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,                // DEVICE descriptor type
    0x0200,                 // USB Spec Release Number in BCD format
    0x00,                   // Class Code
    0x00,                   // Subclass code
    0x00,                   // Protocol code
    USB_EP0_BUFF_SIZE,          // Max packet size for EP0, see usb_config.h
    0x04D8,                 // Vendor ID
    0x0009,                // Product ID: mass storage device demo
    0x0001,                 // Device release number in BCD format
    0x01,                   // Manufacturer string index
    0x02,                   // Product string index
    0x03,                   // Device serial number string index
    0x01                    // Number of possible configurations
};

ROM BYTE configDescriptor_msd[]={
    9,    // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,                // CONFIGURATION descriptor type
    0x20,0x00,          // Total length of data for this cfg
    1,                      // Number of interfaces in this cfg
    1,                      // Index value of this configuration
    0,                      // Configuration string index
    _DEFAULT | _SELF,               // Attributes, see usb_device.h
    50,                     // Max power consumption (2X mA)
                            
    9,   // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,               // INTERFACE descriptor type
    0,                      // Interface Number
    0,                      // Alternate Setting Number
    2,                      // Number of endpoints in this intf
    MSD_INTF,               // Class code
    MSD_INTF_SUBCLASS,      // Subclass code
    MSD_PROTOCOL,             // Protocol code
    0,                      // Interface string index

    7,
    USB_DESCRIPTOR_ENDPOINT,
    _EP01_IN,_BULK,
    MSD_IN_EP_SIZE,0x00,
    0x01,
    
    7,
    USB_DESCRIPTOR_ENDPOINT,
    _EP01_OUT,
    _BULK,
    MSD_OUT_EP_SIZE,0x00,
    0x01
};
#endif
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

#ifdef FS_SUPPORT
//Language code(s) supported string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[1];}sd003={
    sizeof(sd003),
    USB_DESCRIPTOR_STRING,
    {0x0409} //0x0409 = Language ID code for US English
};
//Manufacturer string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[25];}sd004={
sizeof(sd004),USB_DESCRIPTOR_STRING,
{'M','i','c','r','o','c','h','i','p',' ',
'T','e','c','h','n','o','l','o','g','y',' ','I','n','c','.'
}};

//Product string descriptor
ROM struct{BYTE bLength;BYTE bDscType;WORD string[28];}sd005={
sizeof(sd005),USB_DESCRIPTOR_STRING,
{'M','i','c','r','o','c','h','i','p',' ','M','a','s','s',' ','S','t','o','r','a','g','e',' ','D','r','i','v','e'
}};
#endif
//Serial number string descriptor.  Note: This should be unique for each unit 
//built on the assembly line.  Plugging in two units simultaneously with the 
//same serial number into a single machine can cause problems.  Additionally, not 
//all hosts support all character values in the serial number string.  The MSD 
//Bulk Only Transport (BOT) specs v1.0 restrict the serial number to consist only
//of ASCII characters "0" through "9" and capital letters "A" through "F".
ROM struct{BYTE bLength;BYTE bDscType;WORD string[12];}sd006={
sizeof(sd006),USB_DESCRIPTOR_STRING,
{'1','2','3','4','5','6','7','8','9','0','9','9'}};

ROM USB_DEVICE_DESCRIPTOR device_dsc;
ROM USB_DEVICE_DESCRIPTOR *device_dsc_user = &device_dsc_gen;;
ROM BYTE *USB_CD_Ptr[1] = {configDescriptor_gen};
ROM BYTE *USB_SD_Ptr[4] = {&sd000,&sd001,&sd002,NULL};

void SelectUsbConf(BYTE no){
    switch(no){
        case 0:
            device_dsc_user =     &device_dsc_gen;
            
            USB_CD_Ptr[0] = configDescriptor_gen;
            
            USB_SD_Ptr[0] = (BYTE*)&sd000;
            USB_SD_Ptr[1] = (BYTE*)&sd001;
            USB_SD_Ptr[2] = (BYTE*)&sd002;
        break;
#ifdef FS_SUPPORT        
        case 1:
            device_dsc_user =     &device_dsc_msd;
            
            USB_CD_Ptr[0] = configDescriptor_msd;
    
            USB_SD_Ptr[0] = (BYTE*)&sd003;
            USB_SD_Ptr[1] = (BYTE*)&sd004;
            USB_SD_Ptr[2] = (BYTE*)&sd005;
            USB_SD_Ptr[3] = (BYTE*)&sd006;
        break;
#endif            
    }

}