#include "interface/usb.h"
#include "util/bytebuffer.h"
#include "util/log.h"
#include <stdio.h>
#include <stdarg.h>

using openxc::util::bytebuffer::IncomingMessageCallback;

bool USB_PROCESSED = false;
uint8_t LAST_CONTROL_COMMAND_PAYLOAD[256];
size_t LAST_CONTROL_COMMAND_PAYLOAD_LENGTH = 0;;
size_t SENT_BYTES = 0;

void openxc::interface::usb::processSendQueue(UsbDevice* usbDevice) {
    USB_PROCESSED = true;
    for(int i = 0; i < ENDPOINT_COUNT; i++) {
        UsbEndpoint* endpoint = &usbDevice->endpoints[i];
        if(endpoint->direction == UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN) {
            printf("USB endpoint %d buffer:\n", i);
            uint8_t snapshot[QUEUE_LENGTH(uint8_t, &endpoint->queue) + 1];
            QUEUE_SNAPSHOT(uint8_t, &endpoint->queue, snapshot, sizeof(snapshot));
            SENT_BYTES += sizeof(snapshot);
            QUEUE_INIT(uint8_t, &endpoint->queue);
            for(size_t i = 0; i < sizeof(snapshot) - 1; i++) {
                if(snapshot[i] == 0) {
                    printf("\n");
                } else {
                    printf("%c", snapshot[i]);
                }
            }
        }
    }
}

void openxc::interface::usb::initialize(UsbDevice* usbDevice) {
    usb::initializeCommon(usbDevice);
    SENT_BYTES = 0;
}

void openxc::interface::usb::read(UsbDevice* device, UsbEndpoint* endpoint,
        IncomingMessageCallback callback) { }

void openxc::interface::usb::deinitialize(UsbDevice* usbDevice) { }
