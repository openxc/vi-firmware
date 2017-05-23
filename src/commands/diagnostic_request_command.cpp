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


bool openxc::commands::handleDeviceUpdateCommmand(openxc_ControlCommand* command) {
    //put in commands to change LED color
    //lights::enable(lights::LIGHT_A, lights::COLORS.green);
    return true;
}

bool openxc::commands::validateDeviceUpdateCommand(openxc_VehicleMessage* message) {
    bool valid = true;
    if(message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->has_type &&
           command->type == openxc_ControlCommand_Type_UPDATE){
            if(command->has_update_request) {
                openxc_UpdateControlCommand update_request = command->update_request;
                if (update_request.has_action)
                {
                    if (update_request.action == openxc_UpdateControlCommand_Action_START) {
                        if (!update_request.has_size) {
                            valid = false;
                            debug("Size of file not available with start command");
                        }
                        
                    }
                    else if (update_request.action == openxc_UpdateControlCommand_Action_FILE) {
                        if (!(update_request.has_number && update_request.has_data)) {
                            valid = false;
                            debug("count or data not available");
                        }
                    }
                    else if (update_request.action == openxc_UpdateControlCommand_Action_STOP) {
                        //no further validation required here
                    }
                }

            } else {
                valid = false;
                debug("Update request is malformed, missing data");
            }
        } else {
            valid = false;
        }
    } else {
       valid = false;
    }
    if(valid == false)
    {
        debug("OTA: validateUpdateCommand status: false");
    } else {
        debug("OTA: validateUpdateCommand status: true");
    }
    return valid;
}

