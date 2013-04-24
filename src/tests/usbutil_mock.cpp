#include "usbutil.h"
#include "buffers.h"
#include "log.h"

bool USB_PROCESSED = false;

void openxc::usb::processUsbSendQueue(UsbDevice* usbDevice) {
    USB_PROCESSED = true;
}

void openxc::usb::initializeUsb(UsbDevice* usbDevice) {
    initializeUsbCommon(usbDevice);
}

void openxc::usb::readFromHost(UsbDevice* usbDevice, bool (*callback)(uint8_t*)) { }
