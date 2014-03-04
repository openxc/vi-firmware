#include "commands.h"
#include "config.h"
#include "diagnostics.h"
#include "interface/usb.h"
#include "util/log.h"
#include "config.h"
#include "openxc.pb.h"
#include "pb_decode.h"
#include <payload/payload.h>
#include <cJSON.h>

using openxc::interface::usb::sendControlMessage;
using openxc::util::log::debug;
using openxc::config::getConfiguration;
using openxc::payload::PayloadFormat;

namespace payload = openxc::payload;
namespace config = openxc::config;
namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;
namespace uart = openxc::interface::uart;

static bool handleVersionCommand() {
    char descriptor[128];
    config::getFirmwareDescriptor(descriptor, sizeof(descriptor));

    usb::sendControlMessage(&getConfiguration()->usb, (uint8_t*)descriptor,
            strlen(descriptor));
    // TODO inject into outgoing stream, too as COMMAND_RESPONSE type
    return true;
}

static bool handleDeviceIdCommmand() {
    // TODO move getDeviceId to openxc::platform, allow each platform to
    // define where the device ID comes from.
    uart::UartDevice* uart = &getConfiguration()->uart;
    if(strnlen(uart->deviceId, sizeof(uart->deviceId)) > 0) {
        usb::sendControlMessage(&getConfiguration()->usb,
                (uint8_t*)uart->deviceId, strlen(uart->deviceId));
        // TODO inject into outgoing stream, too as COMMAND_RESPONSE type
    }
    return true;
}

static bool handleComplexCommand(uint8_t payload[], size_t payloadLength) {
    openxc_VehicleMessage message;
    bool status = payload::deserialize(payload, payloadLength, &message,
            getConfiguration()->payloadFormat);
    if(status && message.has_control_command) {
        openxc_ControlCommand* command = &message.control_command;
        switch(command->type) {
            case openxc_ControlCommand_Type_DIAGNOSTIC:
                status = diagnostics::handleDiagnosticCommand(
                        &getConfiguration()->diagnosticsManager, command);
                break;
            case openxc_ControlCommand_Type_VERSION:
                status = handleVersionCommand();
                break;
            case openxc_ControlCommand_Type_DEVICE_ID:
                status = handleDeviceIdCommmand();
                break;
            default:
                status = false;
                break;
        }
    }
    return status;
}

bool openxc::commands::handleCommand(Command command, uint8_t payload[],
        int payloadLength) {
    switch(command) {
    case VERSION:
        return handleVersionCommand();
    case DEVICE_ID:
        return handleDeviceIdCommmand();
    case COMPLEX_COMMAND:
        return handleComplexCommand(payload, payloadLength);
    default:
        return false;
    }
}
