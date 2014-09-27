#include "af_bypass_command.h"

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

namespace pipeline = openxc::pipeline;

bool openxc::commands::validateFilterBypassCommand(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->has_acceptance_filter_bypass_command) {
            openxc_AcceptanceFilterBypassCommand* bypassCommand =
                    &command->acceptance_filter_bypass_command;
            if(bypassCommand->has_bus && bypassCommand->has_bypass) {
                valid = true;
            }
        }
    }
    return valid;
}

bool openxc::commands::handleFilterBypassCommand(openxc_ControlCommand* command) {
    bool status = false;
    if(command->has_acceptance_filter_bypass_command) {
        openxc_AcceptanceFilterBypassCommand* bypassCommand =
                &command->acceptance_filter_bypass_command;
        if(bypassCommand->has_bus && bypassCommand->has_bypass) {
            CanBus* bus = NULL;
            if(bypassCommand->has_bus) {
                bus = lookupBus(bypassCommand->bus, getCanBuses(),
                        getCanBusCount());
            } else {
                debug("AF bypass command missing bus");
            }

            if(bus != NULL) {
                openxc::can::setAcceptanceFilterStatus(bus,
                        !bypassCommand->bypass, getCanBuses(), getCanBusCount());
                status = true;
            }
        }
    }

    // TODO could share code with other handlers to send simple success/fail
    // responses that don't require a payload
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = openxc_ControlCommand_Type_PASSTHROUGH;
    message.command_response.has_message = false;
    message.command_response.has_status = true;
    message.command_response.status = status;
    pipeline::publish(&message, &getConfiguration()->pipeline);

    return status;
}
