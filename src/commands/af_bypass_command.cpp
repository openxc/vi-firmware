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
    //if(message->has_control_command) {
    if(message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND) {
        openxc_ControlCommand* command = &message->control_command;
        //if(command->has_acceptance_filter_bypass_command) {
        if(command->type == openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS) {
            openxc_AcceptanceFilterBypassCommand* bypassCommand =
                    &command->acceptance_filter_bypass_command;
            //if(bypassCommand->has_bus && bypassCommand->has_bypass) {
	    if (bypassCommand != NULL) {
                valid = true;
            }
        }
    }
    return valid;
}

bool openxc::commands::handleFilterBypassCommand(openxc_ControlCommand* command) {
    bool status = false;
    //if(command->has_acceptance_filter_bypass_command) {
    if(command->type == openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS) {
        openxc_AcceptanceFilterBypassCommand* bypassCommand =
                &command->acceptance_filter_bypass_command;
        //if(bypassCommand->has_bus && bypassCommand->has_bypass) {
            CanBus* bus = NULL;
            //if(bypassCommand->has_bus) {
                bus = lookupBus(bypassCommand->bus, getCanBuses(),
                        getCanBusCount());
            //} else {
            //    debug("AF bypass command missing bus");
            //}

            if(bus != NULL) {
                openxc::can::setAcceptanceFilterStatus(bus,
                        !bypassCommand->bypass, getCanBuses(), getCanBusCount());
                status = true;
            }
        //}
    }

    sendCommandResponse(openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS,
            status);
    return status;
}
