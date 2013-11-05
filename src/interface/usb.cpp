#include "interface/usb.h"
#include "util/log.h"

using openxc::util::log::debugNoNewline;

void openxc::interface::usb::initializeCommon(UsbDevice* usbDevice) {
    debug("Initializing USB.....");
    QUEUE_INIT(uint8_t, &usbDevice->sendQueue);
    QUEUE_INIT(uint8_t, &usbDevice->receiveQueue);
    usbDevice->configured = false;
    usbDevice->allowRawWrites =
#ifdef USB_ALLOW_RAW_WRITE
        true
#else
        false
#endif
        ;
}
