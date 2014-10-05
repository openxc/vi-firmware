#include "predefined_obd2_command.h"

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

namespace pipeline = openxc::pipeline;

bool openxc::commands::validatePredefinedObd2RequestsCommand(openxc_VehicleMessage* message) {
    return message->has_control_command &&
        message->control_command.has_predefined_obd2_requests_command &&
        message->control_command.predefined_obd2_requests_command.has_enabled;
}

bool openxc::commands::handlePredefinedObd2RequestsCommand(openxc_ControlCommand* command) {
    bool status = false;
    if(command->has_predefined_obd2_requests_command) {
        openxc_PredefinedObd2RequestsCommand* predefinedObd2Command =
                &command->predefined_obd2_requests_command;
        if(predefinedObd2Command->has_enabled) {
            getConfiguration()->recurringObd2Requests = predefinedObd2Command->enabled;
            debug("Set pre-defined, recurring OBD-II requests to %s",
                    predefinedObd2Command->enabled ? "enabled" : "disabled");
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
    message.command_response.type = openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS;
    message.command_response.has_message = false;
    message.command_response.has_status = true;
    message.command_response.status = status;
    pipeline::publish(&message, &getConfiguration()->pipeline);

    return status;
}
