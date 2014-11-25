#include "interface/usb.h"

#include <stdio.h>
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"

#include "util/log.h"
#include "util/bytebuffer.h"
#include "gpio.h"
#include "usb_config.h"
#include "config.h"
#include "emqueue.h"
#include "commands/commands.h"


extern "C" {
#include "bsp.h"
}

#define VBUS_PORT 1
#define VBUS_PIN 30
#define VBUS_FUNCNUM 2

#define USB_DM_PORT 0
#define USB_DM_PIN 30
#define USB_DM_FUNCNUM 1

#define USB_HOST_DETECT_INACTIVE_VALUE 400
#define USB_HOST_DETECT_ACTIVE_VALUE 50

#define USB_CONNECT_PORT 2
#define USB_CONNECT_PIN 9

namespace gpio = openxc::gpio;
namespace commands = openxc::commands;
namespace usb = openxc::interface::usb;

using openxc::config::getConfiguration;
using openxc::util::log::debug;
using openxc::interface::usb::UsbDevice;
using openxc::interface::usb::UsbEndpoint;
using openxc::interface::usb::UsbEndpointDirection;
using openxc::util::bytebuffer::processQueue;
using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_VALUE_LOW;

void configureEndpoints() {
    for(int i = 0; i < ENDPOINT_COUNT; i++) {
        UsbEndpoint* endpoint = &getConfiguration()->usb.endpoints[i];
        Endpoint_ConfigureEndpoint(endpoint->address,
                EP_TYPE_BULK,
                endpoint->direction == UsbEndpointDirection::USB_ENDPOINT_DIRECTION_OUT ?
                        ENDPOINT_DIR_OUT : ENDPOINT_DIR_IN,
                endpoint->size, ENDPOINT_BANK_DOUBLE);
    }
}

extern "C" {

void EVENT_USB_Device_Disconnect() {
    getConfiguration()->usb.configured = false;
}

void EVENT_USB_Device_ControlRequest() {
    if(!(Endpoint_IsSETUPReceived())) {
        return;
    }

    QUEUE_TYPE(uint8_t) payloadQueue;
    QUEUE_INIT(uint8_t, &payloadQueue);

    // Only read payload of our app's control requests, not USB system's
    if((USB_ControlRequest.bmRequestType >> 7 == 0) &&
            USB_ControlRequest.bRequest == CONTROL_COMMAND_REQUEST_ID) {
        Endpoint_ClearSETUP();

        int bytesReceived = 0;
        while(bytesReceived < USB_ControlRequest.wLength) {
            while(!Endpoint_IsOUTReceived());
            while(Endpoint_BytesInEndpoint()) {
                uint8_t byte = Endpoint_Read_8();
                if(!QUEUE_PUSH(uint8_t, &payloadQueue, byte)) {
                    debug("Dropped control request from host -- queue is full");
                    break;
                }
                ++bytesReceived;
            }
            Endpoint_ClearOUT();
        }

        Endpoint_ClearStatusStage();
    }

    int length = QUEUE_LENGTH(uint8_t, &payloadQueue);
    uint8_t snapshot[length];
    if(length > 0) {
        QUEUE_SNAPSHOT(uint8_t, &payloadQueue, snapshot, length);
        openxc::interface::usb::handleIncomingMessage(snapshot, length);
    }
}

void EVENT_USB_Device_ConfigurationChanged(void) {
    getConfiguration()->usb.configured = false;
    configureEndpoints();
    // Don't use debug() in here, because if UART logging is enabled, that
    // breaks most if not all USB control commands because we don't response
    // fast enough.
    getConfiguration()->usb.configured = true;
}

}

/* Private: Flush any queued data out to the USB host. */
static void flushQueueToHost(UsbDevice* usbDevice, UsbEndpoint* endpoint) {
    if(!usb::connected(usbDevice) || QUEUE_EMPTY(uint8_t, &endpoint->queue)) {
        return;
    }

    uint8_t previousEndpoint = Endpoint_GetCurrentEndpoint();
    Endpoint_SelectEndpoint(endpoint->address);
    if(Endpoint_IsINReady()) {
        // get bytes from transmit FIFO into intermediate buffer
        int byteCount = 0;
        while(!QUEUE_EMPTY(uint8_t, &endpoint->queue)
                && byteCount < USB_SEND_BUFFER_SIZE) {
            endpoint->sendBuffer[byteCount++] = QUEUE_POP(uint8_t,
                    &endpoint->queue);
        }

        if(byteCount > 0) {
            Endpoint_Write_Stream_LE(endpoint->sendBuffer, byteCount, NULL);
            Endpoint_ClearIN();
        }
    }
    Endpoint_SelectEndpoint(previousEndpoint);
}

/* Private: Detect if USB VBUS is active.
 *
 * This isn't useful if there's no diode between an external 12v/9v power supply
 * (e.g. vehicle power from OBD-II) and the 5v rail, because then VBUS high when
 * the power is powered on regardless of the status of USB. In that situation,
 * you can fall back to the usbHostDetected() function instead.
 *
 * Returns true if VBUS is high.
 */
static bool vbusDetected() {
    return gpio::getValue(VBUS_PORT, VBUS_PIN) != GPIO_VALUE_LOW;
}

/* Private: Detect if a USB host is actually attached, regardless of VBUS.
 *
 * This is a bit hacky, as normally you should rely on VBUS to detect if USB is
 * connected. See vbusDetected() for reasons why we need this workaround on the
 * current prototype.
 *
 * Returns true of there is measurable activity on the D- USB line.
 */
static bool usbHostDetected(UsbDevice* usbDevice) {
    static int debounce = 0;
    static float average = USB_HOST_DETECT_INACTIVE_VALUE / 2;

    if(gpio::getValue(USB_DM_PORT, USB_DM_PIN) == GPIO_VALUE_LOW) {
        ++debounce;
    } else {
        average = average * .9 + debounce * .1;
        debounce = 0;
    }

    bool hostDetected = true;
    if(!usb::connected(usbDevice) && average < USB_HOST_DETECT_ACTIVE_VALUE) {
        EVENT_USB_Device_ConfigurationChanged();
    }

    if(debounce > USB_HOST_DETECT_INACTIVE_VALUE * 2 ||
            average > USB_HOST_DETECT_INACTIVE_VALUE) {
        debounce = 0;
        average = USB_HOST_DETECT_INACTIVE_VALUE / 2;
        hostDetected = false;
    }
    return hostDetected;
}

/* Private: Configure I/O pins used to detect if USB is connected to a host. */
static void configureUsbDetection() {
    PINSEL_CFG_Type vbusPinConfig;
    vbusPinConfig.Funcnum = VBUS_FUNCNUM;
    vbusPinConfig.Portnum = VBUS_PORT;
    vbusPinConfig.Pinnum = VBUS_PIN;
    vbusPinConfig.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&vbusPinConfig);

    PINSEL_CFG_Type hostDetectPinConfig;
    hostDetectPinConfig.Funcnum = USB_DM_FUNCNUM;
    hostDetectPinConfig.Portnum = USB_DM_PORT;
    hostDetectPinConfig.Pinnum = USB_DM_PIN;
    hostDetectPinConfig.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&hostDetectPinConfig);
}

void openxc::interface::usb::processSendQueue(UsbDevice* usbDevice) {
    USB_USBTask();

    if(!usb::connected(usbDevice)) {
        usbHostDetected(usbDevice);
    }

    if(usb::connected(usbDevice)) {
        if((USB_DeviceState != DEVICE_STATE_Configured || !vbusDetected())
                || !usbHostDetected(usbDevice)) {
            // On Windows the USB device will be configured when plugged in for
            // the first time, regardless of if you are actively using it in an
            // application. Windows will *not* send the USB configured event
            // when an application connects.
            //
            // On Linux and Mac, the USB configured event triggers each time a
            // new connection is made to the device.
            //
            // This means that if vbus is high (i.e. USB *might* be connected),
            // that's the only time we should check the usbHostDetected()
            // workaround. If we call that on Windows when USB is attached, it
            // will *unconfigure* the USB device from the VI side but not
            // reconfigure it until you disconnect and reconnect the device to
            // the PC! If the debounce value is small (which is ideal...) that
            // could happen even before your app has a chance to load the
            // device.
            EVENT_USB_Device_Disconnect();
        } else {
            for(int i = 0; i < ENDPOINT_COUNT; i++) {
                UsbEndpoint* endpoint = &usbDevice->endpoints[i];
                if(endpoint->direction == UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN) {
                    flushQueueToHost(usbDevice, endpoint);
                }
            }
        }
    }
}

void openxc::interface::usb::initialize(UsbDevice* usbDevice) {
    usb::initializeCommon(usbDevice);
    USB_Init();
    ::USB_Connect();
    configureUsbDetection();
}

void openxc::interface::usb::read(UsbDevice* device, UsbEndpoint* endpoint,
        openxc::util::bytebuffer::IncomingMessageCallback callback) {
    uint8_t previousEndpoint = Endpoint_GetCurrentEndpoint();
    Endpoint_SelectEndpoint(endpoint->address);

    bool receivedData = false;
    while(Endpoint_IsOUTReceived()) {
        while(Endpoint_BytesInEndpoint()) {
            if(!QUEUE_PUSH(uint8_t, &endpoint->queue, Endpoint_Read_8())) {
                debug("Dropped write from host -- queue is full");
            }
            receivedData = true;
        }
        Endpoint_ClearOUT();
    }

    if(receivedData) {
        while(processQueue(&endpoint->queue, callback)) {
            continue;
        }
    }

    Endpoint_SelectEndpoint(previousEndpoint);
}

void openxc::interface::usb::deinitialize(UsbDevice* usbDevice) {
    usb::initializeCommon(usbDevice);
    // Turn off USB connection status LED
    gpio::setValue(USB_CONNECT_PORT, USB_CONNECT_PIN, GPIO_VALUE_HIGH);
}
