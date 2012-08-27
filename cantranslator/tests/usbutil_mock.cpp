#include "usbutil.h"
#include "buffers.h"
#include "log.h"

void processInputQueue(UsbDevice* usbDevice) { }

void initializeUsb(UsbDevice* usbDevice) { }

void armForRead(UsbDevice* usbDevice, char* buffer) { }

void readFromHost(UsbDevice* usbDevice, bool (*callback)(uint8_t*)) { }
