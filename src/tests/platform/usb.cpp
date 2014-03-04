#include "interface/usb.h"
#include "util/bytebuffer.h"
#include "util/log.h"

using openxc::commands::IncomingMessageCallback;

bool USB_PROCESSED = false;

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
        uint8_t* data, uint8_t length) {
    return false;
}
