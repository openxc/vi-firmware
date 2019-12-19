#include "passthrough_command.h"

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

bool openxc::commands::validatePassthroughRequest(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->type == openxc_ControlCommand_Type_PASSTHROUGH) {
                valid = true;
        }
    }
    return valid;
}

bool openxc::commands::handlePassthroughModeCommand(openxc_ControlCommand* command) {
    bool status = false;
    if(command->type == openxc_ControlCommand_Type_PASSTHROUGH) {
        openxc_PassthroughModeControlCommand* passthroughRequest =
                &command->passthrough_mode_request;
            CanBus* bus = NULL;
                bus = lookupBus(passthroughRequest->bus, getCanBuses(),
                        getCanBusCount());

            if(bus != NULL) {
                debug("Set passthrough for bus %u to %s", bus->address,
                        passthroughRequest->enabled ? "on" : "off");
                bus->passthroughCanMessages = passthroughRequest->enabled;
                status = true;
            }
    }

    sendCommandResponse(openxc_ControlCommand_Type_PASSTHROUGH, status);
    return status;
}
