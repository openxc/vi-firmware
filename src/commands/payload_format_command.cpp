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

    sendCommandResponse(openxc_ControlCommand_Type_PAYLOAD_FORMAT, status);

    if(status) {
        // Don't change format until we've sent the response
        getConfiguration()->payloadFormat = format;
        debug("Set message format to %s",
                format == PayloadFormat::JSON ? "JSON" : "binary" );
    }

    return status;
}
