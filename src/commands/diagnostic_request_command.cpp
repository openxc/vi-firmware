#include "commands/commands.h"
#include "config.h"
#include "diagnostics.h"
#include "interface/usb.h"
#include "util/log.h"
#include "config.h"
#include "openxc.pb.h"
#include "pb_decode.h"
#include <payload/payload.h>
#include "signals.h"
#include <can/canutil.h>
#include <bitfield/bitfield.h>
#include <limits.h>
#include "commands/diagnostic_request_command.h"

using openxc::interface::usb::sendControlMessage;
using openxc::util::log::debug;
using openxc::config::getConfiguration;
using openxc::payload::PayloadFormat;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalCount;
using openxc::signals::getCommands;
using openxc::signals::getCommandCount;
using openxc::can::lookupBus;
using openxc::can::lookupSignal;

namespace can = openxc::can;
namespace payload = openxc::payload;
namespace config = openxc::config;
namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;
namespace uart = openxc::interface::uart;
namespace pipeline = openxc::pipeline;

bool openxc::commands::handleDiagnosticRequestCommand(openxc_ControlCommand* command) {
    bool status = diagnostics::handleDiagnosticCommand(
            &getConfiguration()->diagnosticsManager, command);

    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = openxc_ControlCommand_Type_DIAGNOSTIC;
    message.command_response.has_message = false;
    message.command_response.has_status = true;
    message.command_response.status = status;
    pipeline::publish(&message, &getConfiguration()->pipeline);

    return status;
}

bool openxc::commands::validateDiagnosticRequest(openxc_VehicleMessage* message) {
    bool valid = true;
    if(message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->has_type &&
                command->type == openxc_ControlCommand_Type_DIAGNOSTIC) {
            openxc_DiagnosticControlCommand* diagControlCommand =
                    &command->diagnostic_request;
            openxc_DiagnosticRequest* request = &diagControlCommand->request;

            if(!diagControlCommand->has_action) {
                valid = false;
                debug("Diagnostic request command missing action");
            }

            if(!request->has_message_id) {
                valid = false;
                debug("Diagnostic request missing message ID");
            }

            if(!request->has_mode) {
                valid = false;
                debug("Diagnostic request missing mode");
            }
        } else {
            valid = false;
            debug("Diagnostic request is of unexpected type");
        }
    } else {
        valid = false;
    }
    return valid;
}

