#include <stdlib.h>
#include "interface/uart.h"
#include "interface/usb.h"
#include "interface/network.h"
#include "pipeline.h"
#include "signals.h"
#include "util/log.h"
#include "lights.h"
#include "util/timer.h"
#include "bluetooth.h"
#include "power.h"
#include "platform/platform.h"
#include <stdio.h>


#define VERSION_CONTROL_COMMAND 0x80
#define RESET_CONTROL_COMMAND 0x81

// USB
#define DATA_IN_ENDPOINT 1
#define DATA_OUT_ENDPOINT 2

namespace uart = openxc::interface::uart;
namespace network = openxc::interface::network;
namespace usb = openxc::interface::usb;
namespace bluetooth = openxc::bluetooth;
namespace lights = openxc::lights;
namespace platform = openxc::platform;
namespace power = openxc::power;
namespace time = openxc::util::time;

using openxc::interface::uart::UartDevice;
using openxc::interface::usb::sendControlMessage;
using openxc::signals::getActiveMessageSet;

extern void reset();
extern void setup();
extern void loop();

const char VERSION[] = "5.1.3-dev";
const int UART_BAUD_RATE = 230400;

UartDevice UART_DEVICE = {UART_BAUD_RATE};
NetworkDevice NETWORK_DEVICE;

UsbDevice USB_DEVICE = {
    DATA_IN_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES,
    DATA_OUT_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES};

Pipeline pipeline = {
#ifdef USE_BINARY_OUTPUT
    openxc::pipeline::PROTO,
#else
    openxc::pipeline::JSON,
#endif
    &USB_DEVICE,
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
    if(uart::connected(pipeline.uart)) {
        lights::enable(lights::LIGHT_B, lights::COLORS.blue);
    } else if(USB_DEVICE.configured) {
        lights::enable(lights::LIGHT_B, lights::COLORS.green);
    } else {
		#ifndef DATA_LOGGER
        lights::disable(lights::LIGHT_B);
		#endif
    }
}

int main(void) {
    platform::initialize();
    openxc::util::log::initialize();
    time::initialize();
    power::initialize();
	usb::initialize(pipeline.usb);
	uart::initialize(pipeline.uart);
    bluetooth::initialize(pipeline.uart);
    network::initialize(pipeline.network);
    lights::initialize();
    debug("Initializing as %s", getActiveMessageSet()->name);
    setup();
    for (;;) {
        loop();
        process(&pipeline);
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
                strlen(getActiveMessageSet()->name) + 4];
        sprintf(combinedVersion, "%s (%s)", VERSION, getActiveMessageSet()->name);
        debug("Version: %s", combinedVersion);

        usb::sendControlMessage((uint8_t*)combinedVersion, strlen(combinedVersion));
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



