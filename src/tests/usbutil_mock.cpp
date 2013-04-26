#include "usbutil.h"
#include "buffers.h"
#include "log.h"

bool USB_PROCESSED = false;

void openxc::interface::usb::processUsbSendQueue(UsbDevice* usbDevice) {
    USB_PROCESSED = true;
}

void openxc::interface::usb::initializeUsb(UsbDevice* usbDevice) {
    initializeUsbCommon(usbDevice);
}

void openxc::interface::usb::readFromHost(UsbDevice* usbDevice, bool (*callback)(uint8_t*)) { }
