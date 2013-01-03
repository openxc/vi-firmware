#include "usbutil.h"
#include "buffers.h"
#include "log.h"

bool USB_PROCESSED = false;

void processUsbSendQueue(UsbDevice* usbDevice) {
    USB_PROCESSED = true;
}

void initializeUsb(UsbDevice* usbDevice) {
    initializeUsbCommon(usbDevice);
}

void armForRead(UsbDevice* usbDevice, char* buffer) { }

void readFromHost(UsbDevice* usbDevice, bool (*callback)(uint8_t*)) { }
