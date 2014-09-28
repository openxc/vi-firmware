#include "payload_format_command.h"
#include "config.h"
#include "util/log.h"
#include "config.h"
#include "openxc.pb.h"
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
    if(message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->has_payload_format_command &&
                command->payload_format_command.has_format) {
            valid = true;
        }
    }
    return valid;
}

bool openxc::commands::handlePayloadFormatCommand(openxc_ControlCommand* command) {
    bool status = false;
    PayloadFormat format;
    if(command->has_payload_format_command) {
        openxc_PayloadFormatCommand* messageFormatCommand =
                &command->payload_format_command;
        if(messageFormatCommand->has_format) {
            switch(messageFormatCommand->format) {
                case openxc_PayloadFormatCommand_PayloadFormat_JSON:
                    format = PayloadFormat::JSON;
                    break;
                case openxc_PayloadFormatCommand_PayloadFormat_PROTOBUF:
                    format = PayloadFormat::PROTOBUF;
                    break;
            }

            status = true;
        }
    }

    // TODO could share code with other handlers to send simple success/fail
    // responses that don't require a payload
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = openxc_ControlCommand_Type_PAYLOAD_FORMAT;
    message.command_response.has_message = false;
    message.command_response.has_status = true;
    message.command_response.status = status;
    pipeline::publish(&message, &getConfiguration()->pipeline);

    if(status) {
        // Don't change format until we've sent the response
        getConfiguration()->payloadFormat = format;
        debug("Set message format to %s",
                format == PayloadFormat::JSON ? "JSON" : "binary" );
    }

    return status;
}
