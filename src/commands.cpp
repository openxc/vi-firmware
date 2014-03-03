#include "commands.h"
#include "config.h"
#include "diagnostics.h"
#include "interface/usb.h"

using openxc::interface::usb::sendControlMessage;

namespace config = openxc::config;
namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;
namespace uart = openxc::interface::uart;

/* Private: Handle an incoming command, which could be from a USB control
 * transfer or a deserilaized command from UART.
 *
 *  - VERSION_CONTROL_COMMAND - return the version of the firmware as a string,
 *      including the vehicle it is built to translate.
 */
bool openxc::commands::handleCommand(uint8_t request, uint8_t payload[],
        int payloadLength) {
    switch(request) {
    case VERSION_CONTROL_COMMAND:
    {
        char descriptor[128];
        config::getFirmwareDescriptor(descriptor, sizeof(descriptor));

        usb::sendControlMessage((uint8_t*)descriptor, strlen(descriptor));
        return true;
    }
    case DEVICE_ID_CONTROL_COMMAND:
    {
        uart::UartDevice* uart = &config::getConfiguration()->uart;
        if(strnlen(uart->deviceId, sizeof(uart->deviceId)) > 0) {
            usb::sendControlMessage((uint8_t*)uart->deviceId,
                    strlen(uart->deviceId));
        }
        return true;
    }
    case DIAGNOSTIC_REQUEST_CONTROL_COMMAND:
    {
        diagnostics::handleDiagnosticCommand(
                &config::getConfiguration()->diagnosticsManager, payload);
        return true;
    }
    default:
        return false;
    }
}
