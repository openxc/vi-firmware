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
    //return message->has_control_command &&
    //    message->control_command.has_predefined_obd2_requests_command &&
    //    message->control_command.predefined_obd2_requests_command.has_enabled;
    return true;
}

bool openxc::commands::handlePredefinedObd2RequestsCommand(openxc_ControlCommand* command) {
    bool status = false;
    //if(command->has_predefined_obd2_requests_command) {
        openxc_PredefinedObd2RequestsCommand* predefinedObd2Command =
                &command->predefined_obd2_requests_command;
        //if(predefinedObd2Command->has_enabled) {
        if (predefinedObd2Command != NULL) {
            getConfiguration()->recurringObd2Requests = predefinedObd2Command->enabled;
            debug("Set pre-defined, recurring OBD-II requests to %s",
                    predefinedObd2Command->enabled ? "enabled" : "disabled");
            status = true;
        }
    //}

    sendCommandResponse(openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS, status);
    return status;
}
