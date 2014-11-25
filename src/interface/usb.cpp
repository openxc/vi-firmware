#include "interface/usb.h"

#include "util/log.h"
#include "commands/commands.h"
#include "config.h"

using openxc::util::log::debug;

void openxc::interface::usb::initializeCommon(UsbDevice* usbDevice) {
    debug("Initializing USB.....");
    for(int i = 0; i < ENDPOINT_COUNT; i++) {
        QUEUE_INIT(uint8_t, &usbDevice->endpoints[i].queue);
    }
    usbDevice->configured = false;
    usbDevice->descriptor.type = InterfaceType::USB;
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

size_t openxc::interface::usb::handleIncomingMessage(uint8_t payload[], size_t length) {
    return commands::handleIncomingMessage(payload, length,
            &config::getConfiguration()->usb.descriptor);
}

bool openxc::interface::usb::connected(UsbDevice* device) {
    return device != NULL && device->configured;
}
