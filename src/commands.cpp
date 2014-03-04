#include "commands.h"
#include "config.h"
#include "diagnostics.h"
#include "interface/usb.h"
#include "util/log.h"
#include "config.h"
#include "openxc.pb.h"
#include "pb_decode.h"
#include <cJSON.h>

#define VERSION_COMMAND_NAME "version"
#define DEVICE_ID_COMMAND_NAME "device_id"
#define DIAGNOSTIC_COMMAND_NAME "diagnostic"

using openxc::interface::usb::sendControlMessage;
using openxc::util::log::debug;
using openxc::config::getConfiguration;

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

static bool deserializeJsonDiagnosticCommand(cJSON* root,
        openxc_ControlCommand* command) {
    command->type = openxc_ControlCommand_Type_DIAGNOSTIC;
    command->has_diagnostic_request = true;

    cJSON* request = cJSON_GetObjectItem(root, "request");
    if(request == NULL) {
        return false;
    }

    cJSON* element = cJSON_GetObjectItem(request, "bus");
    if(element != NULL) {
        command->diagnostic_request.has_bus = true;
        command->diagnostic_request.bus = element->valueint;
    }

    element = cJSON_GetObjectItem(request, "mode");
    if(element != NULL) {
        command->diagnostic_request.has_mode = true;
        command->diagnostic_request.mode = element->valueint;
    }

    element = cJSON_GetObjectItem(request, "id");
    if(element != NULL) {
        command->diagnostic_request.has_message_id = true;
        command->diagnostic_request.message_id = element->valueint;
    }

    element = cJSON_GetObjectItem(request, "pid");
    if(element != NULL) {
        command->diagnostic_request.has_pid = true;
        command->diagnostic_request.pid = element->valueint;
    }

    element = cJSON_GetObjectItem(request, "payload");
    if(element != NULL) {
        command->diagnostic_request.has_payload = true;
        // TODO need to go from hex string to byte array.
        command->diagnostic_request.payload.size = 0;
        // mempcy(command->diagnostic_request.payload.bytes,
                // xxx, xxx.length);
    }

    element = cJSON_GetObjectItem(request, "parse_payload");
    if(element != NULL) {
        command->diagnostic_request.has_parse_payload = true;
        command->diagnostic_request.parse_payload = bool(element->valueint);
    }

    element = cJSON_GetObjectItem(request, "factor");
    if(element != NULL) {
        command->diagnostic_request.has_factor = true;
        command->diagnostic_request.factor = element->valuedouble;
    }

    element = cJSON_GetObjectItem(request, "offset");
    if(element != NULL) {
        command->diagnostic_request.has_offset = true;
        command->diagnostic_request.offset = element->valuedouble;
    }

    element = cJSON_GetObjectItem(request, "frequency");
    if(element != NULL) {
        command->diagnostic_request.has_frequency = true;
        command->diagnostic_request.frequency = element->valuedouble;
    }

    return true;
}

static bool deserializeJsonCommand(uint8_t payload[], size_t length,
        openxc_ControlCommand* command) {
    cJSON *root = cJSON_Parse((char*)payload);
    if(root == NULL) {
        return false;
    }

    bool status = true;
    cJSON* commandNameObject = cJSON_GetObjectItem(root, "command");
    if(commandNameObject != NULL) {
        command->has_type = true;
        if(!strncmp(commandNameObject->valuestring, VERSION_COMMAND_NAME,
                    strlen(VERSION_COMMAND_NAME))) {
            command->type = openxc_ControlCommand_Type_VERSION;
        } else if(!strncmp(commandNameObject->valuestring,
                    DEVICE_ID_COMMAND_NAME, strlen(DEVICE_ID_COMMAND_NAME))) {
            command->type = openxc_ControlCommand_Type_DEVICE_ID;
        } else if(!strncmp(commandNameObject->valuestring,
                    DIAGNOSTIC_COMMAND_NAME, strlen(DIAGNOSTIC_COMMAND_NAME))) {
            status = deserializeJsonDiagnosticCommand(root, command);
        } else {
            debug("Unrecognized command: %s", commandNameObject->valuestring);
            status = false;
        }
    }
    cJSON_Delete(root);
    return status;
}

static bool handleComplexCommand(uint8_t payload[], size_t payloadLength) {
    // TODO parse the payload as JSON or protobuf, depending on the current
    // output format, and store it in the protobuf's struct. Pass that off  to
    // the proper function depending on the requested command name.

    openxc_ControlCommand command;
    bool status = true;
    if(getConfiguration()->outputFormat == openxc::config::JSON) {
        status = deserializeJsonCommand(payload, payloadLength, &command);
    } else {
        pb_istream_t stream = pb_istream_from_buffer(payload, payloadLength);
        status = pb_decode(&stream, openxc_ControlCommand_fields, &command);
        if(!status) {
            debug("Protobuf decoding failed with %s", PB_GET_ERROR(&stream));
        }
    }

    if(status) {
        switch(command.type) {
            case openxc_ControlCommand_Type_DIAGNOSTIC:
                status = diagnostics::handleDiagnosticCommand(
                        &getConfiguration()->diagnosticsManager, &command);
                break;
            case openxc_ControlCommand_Type_VERSION:
                status = handleVersionCommand();
                break;
            case openxc_ControlCommand_Type_DEVICE_ID:
                status  = handleDeviceIdCommmand();
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
