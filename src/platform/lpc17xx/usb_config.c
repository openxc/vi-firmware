#include "usb_config.h"

/** Device descriptor structure. This descriptor, located in FLASH memory,
 * describes the overall device characteristics, including the supported USB
 * version, control endpoint size and the number of device configurations. The
 * descriptor is read out by the USB host when the enumeration process begins.
 */
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
    .Header                 = {.Size = sizeof(USB_Descriptor_Device_t),
                                    .Type = DTYPE_Device},
    .USBSpecification       = USB_VERSION,
    .Class                  = 0,
    .SubClass               = 0,
    .Protocol               = 0,

    .Endpoint0Size          = CONTROL_ENDPOINT_SIZE,

    .VendorID               = VENDOR_ID,
    .ProductID              = PRODUCT_ID,
    .ReleaseNumber          = DEVICE_VERSION,

    .ManufacturerStrIndex   = 0x01,
    .ProductStrIndex        = 0x02,
    .SerialNumStrIndex      = NO_DESCRIPTOR,

    .NumberOfConfigurations = NUM_CONFIGURATIONS
};

/** Configuration descriptor structure. This descriptor, located in FLASH
 * memory, describes the usage of the device in one of its supported
 * configurations, including information about any device interfaces and
 * endpoints. The descriptor is read out by the USB host during the enumeration
 * process when selecting a configuration so that the host may correctly
 * communicate with the USB device.
 */
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {
            .Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                    .Type = DTYPE_Configuration},
            .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
            .TotalInterfaces = INTERFACE_COUNT,
            .ConfigurationNumber = 1,
            .ConfigurationStrIndex = NO_DESCRIPTOR,
            .ConfigAttributes = USB_CONFIG_ATTR_BUSPOWERED,
            .MaxPowerConsumption = USB_CONFIG_POWER_MA(100)
        },

    .Interface = {
            .Header                 = {
                    .Size = sizeof(USB_Descriptor_Interface_t),
                    .Type = DTYPE_Interface},
            .InterfaceNumber        = 0,
            .AlternateSetting       = 0,
            .TotalEndpoints         = ENDPOINT_COUNT,
            .Class                  = 0xff,
            .SubClass               = 0xff,
            .Protocol               = 0xff,
            .InterfaceStrIndex      = NO_DESCRIPTOR
        },

    .InEndpoint = {
            .Header                 = {
                    .Size = sizeof(USB_Descriptor_Endpoint_t),
                    .Type = DTYPE_Endpoint},
            .EndpointAddress        = (ENDPOINT_DIR_IN | IN_ENDPOINT_NUMBER),
            .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
            .EndpointSize           = DATA_ENDPOINT_SIZE,
            .PollingIntervalMS      = 0x01 // unused by bulk endpoints
        },

    .OutEndpoint = {
            .Header                 = {
                .Size = sizeof(USB_Descriptor_Endpoint_t),
                .Type = DTYPE_Endpoint},
            .EndpointAddress        = (ENDPOINT_DIR_OUT | OUT_ENDPOINT_NUMBER),
            .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
            .EndpointSize           = DATA_ENDPOINT_SIZE,
            .PollingIntervalMS      = 0x01 // unused by bulk endpoints
        },

    .LogEndpoint = {
            .Header                 = {
                .Size = sizeof(USB_Descriptor_Endpoint_t),
                .Type = DTYPE_Endpoint},
            .EndpointAddress        = (ENDPOINT_DIR_IN | LOG_ENDPOINT_NUMBER),
            .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
            .EndpointSize           = DATA_ENDPOINT_SIZE,
            .PollingIntervalMS      = 0x01 // unused by bulk endpoints
        }
};

/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests *  the string descriptor with index 0 (the
 * first index). It is actually an array of 16-bit integers, which indicate *
 * via the language ID table available at USB.org what languages the device
 * supports for its string descriptors.
 */
const USB_Descriptor_String_t PROGMEM LanguageString = {
    .Header                 = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},

    .UnicodeString          = {LANGUAGE_ID_ENG}
};

/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 * Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ManufacturerString = {
    .Header                 = {.Size = USB_STRING_LEN(18), .Type = DTYPE_String},
    .UnicodeString          = {
            'F','o','r','d',' ',
            'M','o','t','o','r',' ',
            'C','o','m','p','a','n','y'}
};

/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
const USB_Descriptor_String_t PROGMEM ProductString = {
    .Header                 = {.Size = USB_STRING_LEN(24), .Type = DTYPE_String},

    .UnicodeString          = {
            'O','p','e','n','X','C',' ',
            'V','e','h','i','c','l','e',' ',
            'I','n','t','e','r','f','a','c','e'
    }
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint8_t wIndex,
                                    const void** const DescriptorAddress) {
    const uint8_t  DescriptorType   = (wValue >> 8);
    const uint8_t  DescriptorNumber = (wValue & 0xFF);

    const void* Address = NULL;
    uint16_t    Size    = NO_DESCRIPTOR;

    switch (DescriptorType) {
        case DTYPE_Device:
            Address = &DeviceDescriptor;
            Size    = sizeof(USB_Descriptor_Device_t);
            break;
        case DTYPE_Configuration:
            Address = &ConfigurationDescriptor;
            Size    = sizeof(USB_Descriptor_Configuration_t);
            break;
        case DTYPE_String:
            switch (DescriptorNumber) {
                case 0x00:
                    Address = &LanguageString;
                    Size    = pgm_read_byte(&LanguageString.Header.Size);
                    break;
                case 0x01:
                    Address = &ManufacturerString;
                    Size    = pgm_read_byte(&ManufacturerString.Header.Size);
                    break;
                case 0x02:
                    Address = &ProductString;
                    Size    = pgm_read_byte(&ProductString.Header.Size);
                    break;
            }

            break;
    }

    *DescriptorAddress = Address;
    return Size;
}
