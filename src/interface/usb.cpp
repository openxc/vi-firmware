#include "interface/usb.h"
#include "util/log.h"
#include "commands/commands.h"

using openxc::util::log::debug;

void openxc::interface::usb::initializeCommon(UsbDevice* usbDevice) {
    debug("Initializing USB.....");
    for(int i = 0; i < ENDPOINT_COUNT; i++) {
        QUEUE_INIT(uint8_t, &usbDevice->endpoints[i].queue);
    }
    usbDevice->configured = false;
    usbDevice->allowRawWrites = DEFAULT_ALLOW_RAW_WRITE_USB;
}

void openxc::interface::usb::deinitializeCommon(UsbDevice* usbDevice) {
    usbDevice->configured = false;
}

void openxc::interface::usb::read(UsbDevice* device,
        openxc::util::bytebuffer::IncomingMessageCallback callback) {
    for(int i = 0; i < ENDPOINT_COUNT; i++) {
        UsbEndpoint* endpoint = &device->endpoints[i];
        if(endpoint->direction == UsbEndpointDirection::USB_ENDPOINT_DIRECTION_OUT) {
            read(device, endpoint, callback);
        }
    }
}
