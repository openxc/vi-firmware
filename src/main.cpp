#include <stdlib.h>
#include "interface/uart.h"
#include "interface/usb.h"
#include "interface/network.h"
#include "interface/pipeline.h"
#include "signals.h"
#include "util/log.h"
#include "lights.h"
#include "util/timer.h"
#include "bluetooth.h"
#include "power.h"
#include "platform/platform.h"
#include <stdlib.h>

#define VERSION_CONTROL_COMMAND 0x80
#define RESET_CONTROL_COMMAND 0x81

// USB
#define DATA_IN_ENDPOINT 1
#define DATA_OUT_ENDPOINT 2

using openxc::interface::uart::UartDevice;
using openxc::interface::uart::uartConnected;
using openxc::interface::usb::sendControlMessage;
using openxc::bluetooth::initializeBluetooth;
using openxc::lights::LIGHT_B;
using openxc::lights::COLORS;
using openxc::lights::initializeLights;
using openxc::platform::initializePlatform;
using openxc::power::initializePower;
using openxc::util::time::initializeTimers;
using openxc::util::log::initializeLogging;
using openxc::signals::getMessageSet;

extern void reset();
extern void setup();
extern void loop();

const char* VERSION = "4.0-dev";

UartDevice UART_DEVICE;
NetworkDevice NETWORK_DEVICE;

UsbDevice USB_DEVICE = {
    DATA_IN_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES,
    DATA_OUT_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES};

Listener listener = {&USB_DEVICE,
    &UART_DEVICE,
#ifdef __USE_NETWORK__
    &NETWORK_DEVICE
#endif // __USE_NETWORK__
};

/* Public: Update the color and status of a board's light that shows the output
 * interface status. This function is intended to be called each time through
 * the main program loop.
 */
void updateInterfaceLight() {
    if(uartConnected(listener.uart)) {
        enable(LIGHT_B, COLORS.blue);
    } else if(USB_DEVICE.configured) {
        enable(LIGHT_B, COLORS.green);
    } else {
        disable(LIGHT_B);
    }
}

int main(void) {
    initializePlatform();
    initializeLogging();
    initializeTimers();
    initializePower();
    initializeUsb(listener.usb);
    initializeUart(listener.uart);
    initializeNetwork(listener.network);
    initializeLights();
    initializeBluetooth();

    debug("Initializing as %s", getMessageSet());
    setup();

    for (;;) {
        loop();
        processListenerQueues(&listener);
        updateInterfaceLight();
    }

    return 0;
}

/* Private: Handle an incoming USB control request.
 *
 * There are two accepted control requests:
 *
 *  - VERSION_CONTROL_COMMAND - return the version of the firmware as a string,
 *      including the vehicle it is built to translate.
 *  - RESET_CONTROL_COMMAND - reset the device.
 *
 *  TODO This function is defined in main.cpp because it needs to reference the
 *  version and message set, which aren't declared in any header files at the
 *  moment. Ripe for refactoring!
 */
bool handleControlRequest(uint8_t request) {
    switch(request) {
    case VERSION_CONTROL_COMMAND:
    {
        char combinedVersion[strlen(VERSION) +
                strlen(getMessageSet()) + 4];
        sprintf(combinedVersion, "%s (%s)", VERSION, getMessageSet());
        debug("Version: %s", combinedVersion);

        sendControlMessage((uint8_t*)combinedVersion, strlen(combinedVersion));
        return true;
    }
    case RESET_CONTROL_COMMAND:
        debug("Resetting...");
        reset();
        return true;
    default:
        return false;
    }
}
