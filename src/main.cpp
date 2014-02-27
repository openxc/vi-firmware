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
#include "diagnostics.h"
#include <stdlib.h>

#define VERSION_CONTROL_COMMAND 0x80
#define DEVICE_ID_CONTROL_COMMAND 0x82
#define DIAGNOSTIC_REQUEST_CONTROL_COMMAND 0x83

namespace uart = openxc::interface::uart;
namespace network = openxc::interface::network;
namespace usb = openxc::interface::usb;
namespace bluetooth = openxc::bluetooth;
namespace lights = openxc::lights;
namespace platform = openxc::platform;
namespace power = openxc::power;
namespace time = openxc::util::time;
namespace diagnostics = openxc::diagnostics;

using openxc::pipeline::Pipeline;
using openxc::util::log::debug;
using openxc::interface::uart::UartDevice;
using openxc::interface::usb::sendControlMessage;
using openxc::signals::getActiveMessageSet;

extern void setup();
extern void loop();

const char VERSION[] = "6.0-dev";
const int UART_BAUD_RATE = 230400;

UartDevice UART_DEVICE = {UART_BAUD_RATE};
NetworkDevice NETWORK_DEVICE;

UsbDevice USB_DEVICE = {
    {
        {IN_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE, usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN},
        {OUT_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE, usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_OUT},
        {LOG_ENDPOINT_NUMBER, DATA_ENDPOINT_SIZE, usb::UsbEndpointDirection::USB_ENDPOINT_DIRECTION_IN},
    }
};

Pipeline PIPELINE = {
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

diagnostics::DiagnosticsManager DIAGNOSTICS_MANAGER;

/* Public: Update the color and status of a board's light that shows the output
 * interface status. This function is intended to be called each time through
 * the main program loop.
 */
void updateInterfaceLight() {
    if(uart::connected(PIPELINE.uart)) {
        lights::enable(lights::LIGHT_B, lights::COLORS.blue);
    } else if(USB_DEVICE.configured) {
        lights::enable(lights::LIGHT_B, lights::COLORS.green);
    } else {
        lights::disable(lights::LIGHT_B);
    }
}

int main(void) {
    platform::initialize();
    openxc::util::log::initialize();
    time::initialize();
    lights::initialize();
    power::initialize();
    usb::initialize(PIPELINE.usb);
    uart::initialize(PIPELINE.uart);
    updateInterfaceLight();
    // give basic power indication ASAP, even if no CAN activity or output
    // interface attached
    lights::enable(lights::LIGHT_A, lights::COLORS.red);

    bluetooth::initialize(PIPELINE.uart);
    network::initialize(PIPELINE.network);

    srand(time::systemTimeMs());

    debug("Initializing as %s", getActiveMessageSet()->name);
    setup();

    for (;;) {
        loop();
        process(&PIPELINE);
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
 *
 *  TODO This function is defined in main.cpp because it needs to reference the
 *  version and message set, which aren't declared in any header files at the
 *  moment. Ripe for refactoring!
 */
bool handleControlRequest(uint8_t request, uint8_t payload[], int payloadLength) {
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
    case DEVICE_ID_CONTROL_COMMAND:
    {
        if(strnlen(UART_DEVICE.deviceId, sizeof(UART_DEVICE.deviceId)) > 0) {
            debug("Device ID: %s", UART_DEVICE.deviceId);
            usb::sendControlMessage((uint8_t*)UART_DEVICE.deviceId,
                    strlen(UART_DEVICE.deviceId));
        }
        return true;
    }
    case DIAGNOSTIC_REQUEST_CONTROL_COMMAND:
    {
        diagnostics::handleDiagnosticCommand(&DIAGNOSTICS_MANAGER, payload);
        return true;
    }
    default:
        return false;
    }
}
