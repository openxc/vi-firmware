#include "commands/commands.h"

#include "config.h"
#include "util/log.h"
#include "interface/interface.h"
#include "config.h"
#include "pb_decode.h"

#include "commands/passthrough_command.h"
#include "commands/diagnostic_request_command.h"
#include "commands/version_command.h"
#include "commands/device_id_command.h"
#include "commands/device_platform_command.h"
//#include "commands/device_update_command.h"
#include "commands/can_message_write_command.h"
#include "commands/simple_write_command.h"
#include "commands/af_bypass_command.h"
#include "commands/payload_format_command.h"
#include "commands/predefined_obd2_command.h"
#include "commands/modem_config_command.h"
#include "commands/rtc_config_command.h"
#include "commands/sd_mount_status_command.h"
#include "lights.h"

using openxc::util::log::debug;
using openxc::config::getConfiguration;
using openxc::payload::PayloadFormat;
using openxc::interface::InterfaceType;
/***************************/
namespace lights = openxc::lights;

double firmwareUpdateSize = 0;
uint32_t count;
openxc_UpdateMessage_data_t file;
/***************************/

static bool handleComplexCommand(openxc_VehicleMessage* message) {
    bool status = true;
    if(message != NULL && message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        switch(command->type) {
        case openxc_ControlCommand_Type_DIAGNOSTIC:
            status = openxc::commands::handleDiagnosticRequestCommand(command);
            break;
        case openxc_ControlCommand_Type_VERSION:
            status = openxc::commands::handleVersionCommand();
            break;
        case openxc_ControlCommand_Type_DEVICE_ID:
            status = openxc::commands::handleDeviceIdCommmand();
            break;
        case openxc_ControlCommand_Type_PLATFORM:
            status = openxc::commands::handleDevicePlatformCommmand();
            break;
        case openxc_ControlCommand_Type_UPDATE:
             /*lights::enable(lights::LIGHT_A, lights::COLORS.green);
             status = true;*/
             //status = openxc::commands::handleDeviceUpdateCommand(command);
            {
             //when the whole firmware is received, do IAP
             //Start command gives file size
             //use a static variable to add on to the byte array
             //EOF msg to know when to do the IAP
                debug("OTA: Update command being handled");
                openxc_UpdateControlCommand update_request = command->update_request;
                static PB_BYTES_ARRAY_T(4000) firmwareFile;
                if (update_request.has_action)
                {
                    if (update_request.action == openxc_UpdateControlCommand_Action_START) {
                        // create array to store the file that's coming next
                        //TODO: dynamic allocation with checks for runtime failures
                        firmwareFile.size = update_request.size;;
                        
                    }
                    else if (update_request.action == openxc_UpdateControlCommand_Action_FILE) {
                        //int count = update_request.number;
                        // copy data
                        //memcpy(&firmwareFile.bytes[(count - 1) * 128],&update_request.data.bytes, update_request.data.size);
                    }
                    else if (update_request.action == openxc_UpdateControlCommand_Action_STOP) {
                        //file has been received.
                        //TODO: check if received == file size that was given
                        //TODO: return a response to the phone
                        //perform IAP
                        
                    }
                }
                
                lights::enable(lights::LIGHT_A, lights::COLORS.green);
                
                
            
            }
            break;
        case openxc_ControlCommand_Type_PASSTHROUGH:
            status = openxc::commands::handlePassthroughModeCommand(command);
            break;
        case openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS:
            status = openxc::commands::handlePredefinedObd2RequestsCommand(command);
            break;
        case openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS:
            status = openxc::commands::handleFilterBypassCommand(command);
            break;
        case openxc_ControlCommand_Type_PAYLOAD_FORMAT:
            status = openxc::commands::handlePayloadFormatCommand(command);
            break;
        case openxc_ControlCommand_Type_MODEM_CONFIGURATION:
            status = openxc::commands::handleModemConfigurationCommand(command);
            break;
        case openxc_ControlCommand_Type_RTC_CONFIGURATION:
            status = openxc::commands::handleRTCConfigurationCommand(command);
        break;
        case openxc_ControlCommand_Type_SD_MOUNT_STATUS:
            status =  openxc::commands::handleSDMountStatusCommand();
        default:
            status = false;
            break;
        }
    }
    return status;
}

size_t openxc::commands::handleIncomingMessage(uint8_t payload[], size_t length,
        openxc::interface::InterfaceDescriptor* sourceInterfaceDescriptor) {
    openxc_VehicleMessage message = {0};
    size_t bytesRead = 0;

    // TODO Not attempting to deserialize binary messages via UART,
    // see https://github.com/openxc/vi-firmware/issues/313
    if(sourceInterfaceDescriptor->type == InterfaceType::UART &&
            getConfiguration()->payloadFormat == PayloadFormat::PROTOBUF) {
        return 0;
    }

    // Ignore anything less than 2 bytes, we know it's an incomplete payload -
    // wait for more to come in before trying to parse it
    if(length > 2) {
        if((bytesRead = openxc::payload::deserialize(payload, length,
                getConfiguration()->payloadFormat, &message)) > 0) {
            if(validate(&message)) {
                switch(message.type) {
                case openxc_VehicleMessage_Type_CAN:
                    handleCan(&message, sourceInterfaceDescriptor);
                    break;
                case openxc_VehicleMessage_Type_SIMPLE:
                    handleSimple(&message);
                    break;
                case openxc_VehicleMessage_Type_CONTROL_COMMAND:
                        debug("OTA: Control command being handled");
                    handleComplexCommand(&message);
                    break;
                default:
                    debug("Incoming message had unrecognized type: %d", message.type);
                    break;
                }
            } else {
                lights::enable(lights::LIGHT_A, lights::COLORS.red);
                debug("Incoming message is complete but invalid");
            }
        } else {
            // This is very noisy when using UART as the packet tends to arrive
            // in a couple or bursts and is passed to this function when
            // incomplete.
            // debug("Unable to deserialize a %s message from the payload",
                 // getConfiguration()->payloadFormat == PayloadFormat::JSON ?
                     // "JSON" : "Protobuf");
        }
    }
    return bytesRead;
}

static bool validateControlCommand(openxc_VehicleMessage* message) {
    bool valid = message->has_type &&
            message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND &&
            message->has_control_command &&
            message->control_command.has_type;
    if(valid) {
        switch(message->control_command.type) {
        case openxc_ControlCommand_Type_DIAGNOSTIC:
            valid = openxc::commands::validateDiagnosticRequest(message);
            break;
        case openxc_ControlCommand_Type_PASSTHROUGH:
            valid = openxc::commands::validatePassthroughRequest(message);
            break;
        case openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS:
            valid = openxc::commands::validatePredefinedObd2RequestsCommand(message);
            break;
        case openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS:
            valid = openxc::commands::validateFilterBypassCommand(message);
            break;
        case openxc_ControlCommand_Type_PAYLOAD_FORMAT:
            valid = openxc::commands::validatePayloadFormatCommand(message);
            break;
        case openxc_ControlCommand_Type_UPDATE:
            valid = openxc::commands::validateDeviceUpdateCommand(message);
            break;
        case openxc_ControlCommand_Type_VERSION:
        case openxc_ControlCommand_Type_DEVICE_ID:
        case openxc_ControlCommand_Type_PLATFORM:
        case openxc_ControlCommand_Type_SD_MOUNT_STATUS:
            valid =  true;
            break;
        case openxc_ControlCommand_Type_MODEM_CONFIGURATION:
            valid = openxc::commands::validateModemConfigurationCommand(message);
            break;
        case openxc_ControlCommand_Type_RTC_CONFIGURATION:
            valid = openxc::commands::validateRTCConfigurationCommand(message);
            break;    
        default:
            valid = false;
            break;
        }
    }
    if(valid == false)
    {
        debug("OTA: validateControlCommand status: false");
    } else {
        debug("OTA: validateControlCommand status: true");
    }
    return valid;
}

bool openxc::commands::validate(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message != NULL && message->has_type) {
        switch(message->type) {
        case openxc_VehicleMessage_Type_CAN:
            valid = validateCan(message);
            break;
        case openxc_VehicleMessage_Type_SIMPLE:
            valid = validateSimple(message);
            break;
        case openxc_VehicleMessage_Type_CONTROL_COMMAND:
            valid = validateControlCommand(message);
            break;
        default:
            debug("Incoming message had unrecognized type: %d", message->type);
            break;
        }
    }
    return valid;
}

void openxc::commands::sendCommandResponse(openxc_ControlCommand_Type commandType,
        bool status, char* responseMessage, size_t responseMessageLength) {
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = commandType;
    message.command_response.has_message = false;
    message.command_response.has_status = true;
    message.command_response.status = status;
    if(responseMessage != NULL && responseMessageLength > 0) {
        message.command_response.has_message = true;
        strncpy(message.command_response.message, responseMessage,
                MIN(sizeof(message.command_response.message),
                    responseMessageLength));
    }
    pipeline::publish(&message, &getConfiguration()->pipeline);
}

void openxc::commands::sendCommandResponse(openxc_ControlCommand_Type commandType,
        bool status) {
    sendCommandResponse(commandType, status, NULL, 0);
}
