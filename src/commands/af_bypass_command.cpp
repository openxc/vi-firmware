#include "af_bypass_command.h"

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

bool openxc::commands::validateFilterBypassCommand(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->type == openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS) {
            openxc_AcceptanceFilterBypassCommand* bypassCommand =
                    &command->acceptance_filter_bypass_command;
	    if (bypassCommand != NULL) {
                valid = true;
            }
        }
    }
    return valid;
}

bool openxc::commands::handleFilterBypassCommand(openxc_ControlCommand* command) {
    bool status = false;
    if(command->type == openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS) {
        openxc_AcceptanceFilterBypassCommand* bypassCommand =
                &command->acceptance_filter_bypass_command;
            CanBus* bus = NULL;
                bus = lookupBus(bypassCommand->bus, getCanBuses(),
                        getCanBusCount());

            if(bus != NULL) {
                openxc::can::setAcceptanceFilterStatus(bus,
                        !bypassCommand->bypass, getCanBuses(), getCanBusCount());
                status = true;
            }
    }

    sendCommandResponse(openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS,
            status);
    return status;
}
