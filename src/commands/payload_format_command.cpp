#include "payload_format_command.h"

#include "config.h"
#include "util/log.h"
#include "config.h"
#include "signals.h"
#include <can/canutil.h>

using openxc::util::log::debug;
using openxc::config::getConfiguration;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::can::lookupBus;
using openxc::payload::PayloadFormat;

namespace pipeline = openxc::pipeline;

bool openxc::commands::validatePayloadFormatCommand(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->type == openxc_ControlCommand_Type_PAYLOAD_FORMAT &&
                command->payload_format_command.format != openxc_PayloadFormatCommand_PayloadFormat_UNUSED) {
            valid = true;
        }
    }
    return valid;
}

bool openxc::commands::handlePayloadFormatCommand(openxc_ControlCommand* command) {
    bool status = false;
    PayloadFormat format;
    if(command->type == openxc_ControlCommand_Type_PAYLOAD_FORMAT) {
        openxc_PayloadFormatCommand* messageFormatCommand =
                &command->payload_format_command;
        if(messageFormatCommand->format != openxc_PayloadFormatCommand_PayloadFormat_UNUSED) {
            switch(messageFormatCommand->format) {
                default:
                case openxc_PayloadFormatCommand_PayloadFormat_JSON:
                    format = PayloadFormat::JSON;
                    break;
                case openxc_PayloadFormatCommand_PayloadFormat_PROTOBUF:
                    format = PayloadFormat::PROTOBUF;
                    break;
                case openxc_PayloadFormatCommand_PayloadFormat_MESSAGEPACK:
                    format = PayloadFormat::MESSAGEPACK;
                    break;    
            }

            status = true;
        }
    }

    sendCommandResponse(openxc_ControlCommand_Type_PAYLOAD_FORMAT, status);

    if(status) {
        // Don't change format until we've sent the response
        getConfiguration()->payloadFormat = format;
        debug("Set message format to %s",
                format == PayloadFormat::JSON ? "JSON" : "binary" );
    }

    return status;
}
