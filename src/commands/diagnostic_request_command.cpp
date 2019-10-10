#include "commands/diagnostic_request_command.h"

#include "config.h"
#include "diagnostics.h"
#include "interface/usb.h"
#include "util/log.h"
#include "config.h"
#include "pb_decode.h"
#include <payload/payload.h>
#include "signals.h"
#include <can/canutil.h>
#include <bitfield/bitfield.h>
#include <limits.h>

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
    sendCommandResponse(openxc_ControlCommand_Type_DIAGNOSTIC, status);
    return status;
}

bool openxc::commands::validateDiagnosticRequest(openxc_VehicleMessage* message) {
    bool valid = true;
    //if(message->has_control_command) {
    if(message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND) {
        openxc_ControlCommand* command = &message->control_command;
        //if(command->has_type && command->type == openxc_ControlCommand_Type_DIAGNOSTIC) {
        if(command->type == openxc_ControlCommand_Type_DIAGNOSTIC) {
            openxc_DiagnosticControlCommand* diagControlCommand =
                    &command->diagnostic_request;
            openxc_DiagnosticRequest* request = &diagControlCommand->request;

            //if(!diagControlCommand->has_action) {
            if(diagControlCommand->action == 0) {
                valid = false;
                debug("Diagnostic request command missing action");
            }

            //if(!request->has_message_id) {
            if(request->message_id == 0) {
                valid = false;
                debug("Diagnostic request missing message ID");
            }

            //if(!request->has_mode) {
            if(request->mode == 0) {
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

