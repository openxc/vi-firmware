#include "usbutil.h"
#include "log.h"

void initializeUsbCommon(UsbDevice* usbDevice) {
    debugNoNewline("Initializing USB.....");
    QUEUE_INIT(uint8_t, &usbDevice->sendQueue);
    QUEUE_INIT(uint8_t, &usbDevice->receiveQueue);
    usbDevice->configured = false;
}
