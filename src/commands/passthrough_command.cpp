#include "passthrough_command.h"
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

bool openxc::commands::validatePassthroughRequest(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->has_passthrough_mode_request) {
            openxc_PassthroughModeControlCommand* passthroughRequest =
                    &command->passthrough_mode_request;
            if(passthroughRequest->has_bus && passthroughRequest->has_enabled) {
                valid = true;
            }
        }
    }
    return valid;
}

bool openxc::commands::handlePassthroughModeCommand(openxc_ControlCommand* command) {
    bool status = false;
    if(command->has_passthrough_mode_request) {
        openxc_PassthroughModeControlCommand* passthroughRequest =
                &command->passthrough_mode_request;
        if(passthroughRequest->has_bus && passthroughRequest->has_enabled) {
            CanBus* bus = NULL;
            if(passthroughRequest->has_bus) {
                bus = lookupBus(passthroughRequest->bus, getCanBuses(),
                        getCanBusCount());
            } else {
                debug("Passthrough mode request missing bus");
            }

            if(bus != NULL) {
                debug("Set passthrough for bus %u to %s", bus->address,
                        passthroughRequest->enabled ? "on" : "off");
                bus->passthroughCanMessages = passthroughRequest->enabled;
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
