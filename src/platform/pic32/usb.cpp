#include "interface/usb.h"
#include "util/bytebuffer.h"
#include "util/log.h"
#include "power.h"
#include "config.h"
#include "gpio.h"

#define USB_HANDLE_MAX_WAIT_COUNT 1000

namespace gpio = openxc::gpio;
namespace commands = openxc::commands;

using openxc::util::log::debug;
using openxc::interface::usb::UsbDevice;
using openxc::interface::usb::UsbEndpoint;
using openxc::interface::usb::UsbEndpointDirection;
using openxc::gpio::GPIO_DIRECTION_INPUT;
using openxc::util::bytebuffer::processQueue;
using openxc::config::getConfiguration;

// This is a reference to the last packet read
extern volatile CTRL_TRF_SETUP SetupPkt;

/* Private: Arm the given endpoint for a read from the device to host.
 *
 * This also puts a NUL char in the beginning of the buffer so you don't get
 * confused that it's still a valid message.
 *
 * device - the CAN USB device to arm the endpoint on
 * endpoint - the endpoint to arm.
 * buffer - the destination buffer for the next OUT transfer.
 */
static void armForRead(UsbDevice* usbDevice, UsbEndpoint* endpoint) {
    endpoint->receiveBuffer[0] = 0;
    endpoint->hostToDeviceHandle = usbDevice->device.GenRead(
            endpoint->address, (uint8_t*)endpoint->receiveBuffer,
            endpoint->size);
}

static uint8_t INCOMING_EP0_DATA_BUFFER[256];
static size_t INCOMING_EP0_DATA_SIZE;
static void handleCompletedEP0OutTransfer() {
    commands::handleControlCommand(commands::Command(SetupPkt.bRequest),
            INCOMING_EP0_DATA_BUFFER, INCOMING_EP0_DATA_SIZE);
    memset(INCOMING_EP0_DATA_BUFFER, sizeof(INCOMING_EP0_DATA_BUFFER), 0);
}

boolean usbCallback(USB_EVENT event, void *pdata, word size) {
    // initial connection up to configure will be handled by the default
    // callback routine.
    getConfiguration()->usb.device.DefaultCBEventHandler(event, pdata, size);

    switch(event) {
    case EVENT_CONFIGURED:
        debug("USB Configured");
        getConfiguration()->usb.configured = true;

        for(int i = 0; i < ENDPOINT_COUNT; i++) {
            UsbEndpoint* endpoint = &getConfiguration()->usb.endpoints[i];
            if(endpoint->direction == UsbEndpointDirection::USB_ENDPOINT_DIRECTION_OUT) {
                getConfiguration()->usb.device.EnableEndpoint(endpoint->address,
                        USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
                armForRead(&getConfiguration()->usb, endpoint);
            } else {
                getConfiguration()->usb.device.EnableEndpoint(endpoint->address,
                        USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
            }
        }
        break;

    case EVENT_EP0_REQUEST:
    {
        if((SetupPkt.bmRequestType >> 7 == 0) && SetupPkt.bRequest >= 0x80
                && SetupPkt.wLength > 0) {
            getConfiguration()->usb.device.EP0Receive(INCOMING_EP0_DATA_BUFFER,
                    MIN(SetupPkt.wLength, sizeof(INCOMING_EP0_DATA_BUFFER)),
                        (void*)handleCompletedEP0OutTransfer);
        } else {
            commands::handleControlCommand(commands::Command(SetupPkt.bRequest),
                    NULL, 0);
        }

        break;
    }

    case EVENT_SUSPEND:
        if(getConfiguration()->usb.configured) {
            debug("USB no longer detected - marking unconfigured");
            getConfiguration()->usb.configured = false;
        }
        break;

    default:
        break;
    }
    return true;
}

bool openxc::interface::usb::sendControlMessage(UsbDevice* usbDevice,
        uint8_t* data, size_t length) {
    if(usbDevice->configured) {
        usbDevice->device.EP0SendRAMPtr(data, length, USB_EP0_INCLUDE_ZERO);
        return true;
    }
    return false;
}

bool waitForHandle(UsbDevice* usbDevice, UsbEndpoint* endpoint) {
    int i = 0;
    while(usbDevice->configured &&
            usbDevice->device.HandleBusy(endpoint->deviceToHostHandle)) {
        if(++i > USB_HANDLE_MAX_WAIT_COUNT) {
            // The reason we want to exit this loop early is that if USB is
            // attached and configured, but the host isn't sending an IN
            // requests, we will block here forever. As it is, it still slows
            // down UART transfers quite a bit, so setting configured = false
            // ASAP is important.
            //
            // TODO If USB is attached but not reading, this can block so much
            // that the watchdog doesn't get fed. There has to be a better way
            // to use USB on the PIC32!
            openxc::power::feedWatchdog();
            return false;
        }
    }
    return true;
}

void openxc::interface::usb::processSendQueue(UsbDevice* usbDevice) {
    for(int i = 0; i < ENDPOINT_COUNT; i++) {
        UsbEndpoint* endpoint = &usbDevice->endpoints[i];

        // Don't touch usbDevice->sendBuffer if there's still a pending transfer
        if(!waitForHandle(usbDevice, endpoint)) {
            return;
        }

        while(usbDevice->configured &&
                !QUEUE_EMPTY(uint8_t, &endpoint->queue)) {
            int byteCount = 0;
            while(!QUEUE_EMPTY(uint8_t, &endpoint->queue) &&
                    byteCount < USB_SEND_BUFFER_SIZE) {
                endpoint->sendBuffer[byteCount++] = QUEUE_POP(uint8_t,
                        &endpoint->queue);
            }

            int nextByteIndex = 0;
            while(nextByteIndex < byteCount) {
                // Make sure the USB write is 100% complete before messing with this
                // buffer after we copy the message into it - the Microchip library
                // doesn't copy the data to its own internal buffer. See #171 for
                // background on this issue.
                if(!waitForHandle(usbDevice, endpoint)) {
                    debug("USB not responding in a timely fashion, dropped data");
                    return;
                }

                int bytesToTransfer = min(MAX_USB_PACKET_SIZE_BYTES,
                        byteCount - nextByteIndex);
                endpoint->deviceToHostHandle = usbDevice->device.GenWrite(
                        endpoint->address,
                        &endpoint->sendBuffer[nextByteIndex], bytesToTransfer);
                nextByteIndex += bytesToTransfer;
            }
        }
    }
}

void openxc::interface::usb::initialize(UsbDevice* usbDevice) {
    usb::initializeCommon(usbDevice);
    usbDevice->device = USBDevice(usbCallback);
    usbDevice->device.InitializeSystem(false);
    debug("Done.");
}

void openxc::interface::usb::deinitialize(UsbDevice* usbDevice) {
    // disable USB (notifies stack we are disabling)
    USBModuleDisable();

    // Could not find a ready-made function or macro
    // in the USB library to actually turn off the module.
    // USBModuleDisable() is close to what we want, but it
    // sets the ON bit to 1 for some reason.
    // So, easy solution is just go right to the control register to power off
    // the USB peripheral.
    U1PWRCCLR = (1 << 0);
}

void openxc::interface::usb::read(UsbDevice* device, UsbEndpoint* endpoint,
        commands::IncomingMessageCallback callback) {
    if(!device->device.HandleBusy(endpoint->hostToDeviceHandle)) {
        if(endpoint->receiveBuffer[0] != '\0') {
            for(int i = 0; i < endpoint->size; i++) {
                if(!QUEUE_PUSH(uint8_t, &endpoint->queue,
                            endpoint->receiveBuffer[i])) {
                    debug("Dropped write from host -- queue is full");
                }
            }
            processQueue(&endpoint->queue, callback);
        }
        armForRead(device, endpoint);
    }
}
