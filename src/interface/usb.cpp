#include "interface/usb.h"
#include "util/log.h"

using openxc::util::log::debug;

void openxc::interface::usb::initializeCommon(UsbDevice* usbDevice) {
    debug("Initializing USB.....");
    for(int i = 0; i < ENDPOINT_COUNT; i++) {
        QUEUE_INIT(uint8_t, &usbDevice->endpoints[i].queue);
    }
    usbDevice->configured = false;
    usbDevice->allowRawWrites =
#ifdef USB_ALLOW_RAW_WRITE
        true
#else
        false
#endif
        ;
}

void openxc::interface::usb::deinitializeCommon(UsbDevice* usbDevice) {
    usbDevice->configured = false;
}

void openxc::interface::usb::read(UsbDevice* device,
        bool (*callback)(uint8_t*)) {
    for(int i = 0; i < ENDPOINT_COUNT; i++) {
        UsbEndpoint* endpoint = &device->endpoints[i];
        if(endpoint->direction == UsbEndpointDirection::USB_ENDPOINT_DIRECTION_OUT) {
            read(device, endpoint, callback);
        }
    }
}
