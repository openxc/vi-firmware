#include "interface/usb.h"
#include "util/bytebuffer.h"
#include "util/log.h"

using openxc::commands::IncomingMessageCallback;

bool USB_PROCESSED = false;
uint8_t LAST_CONTROL_COMMAND_PAYLOAD[256];
size_t LAST_CONTROL_COMMAND_PAYLOAD_LENGTH = 0;;

void openxc::interface::usb::processSendQueue(UsbDevice* usbDevice) {
    USB_PROCESSED = true;
}

void openxc::interface::usb::initialize(UsbDevice* usbDevice) {
    usb::initializeCommon(usbDevice);
}

void openxc::interface::usb::read(UsbDevice* device, UsbEndpoint* endpoint,
        IncomingMessageCallback callback) { }

void openxc::interface::usb::deinitialize(UsbDevice* usbDevice) { }

bool openxc::interface::usb::sendControlMessage(UsbDevice* usbDevice,
        uint8_t* data, size_t length) {
    memcpy(LAST_CONTROL_COMMAND_PAYLOAD, data, length);
    LAST_CONTROL_COMMAND_PAYLOAD_LENGTH = length;
    return true;
}
