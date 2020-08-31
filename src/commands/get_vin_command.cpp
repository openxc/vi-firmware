#include "get_vin_command.h"

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
#include <openxc.pb.h>

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

bool openxc::commands::handleGetVinCommand() {
    char* vin;

    if (openxc::diagnostics::haveVINfromCan()) {
    	vin = (char *)openxc::diagnostics::getVIN();
        sendCommandResponse(openxc_ControlCommand_Type_GET_VIN, true, vin, strlen(vin));
    } else {
        openxc_ControlCommand command = openxc_ControlCommand();	// Zero Fill
        command.type = openxc_ControlCommand_Type_DIAGNOSTIC;

        command.type = openxc_ControlCommand_Type_DIAGNOSTIC;
        command.diagnostic_request.action =
                    openxc_DiagnosticControlCommand_Action_ADD;

        command.diagnostic_request.request.bus = 1;
        command.diagnostic_request.request.mode = 9;
        command.diagnostic_request.request.message_id = 0x7e0;
        command.diagnostic_request.request.pid = 2;
        openxc::diagnostics::setVinCommandInProgress(true);

        diagnostics::handleDiagnosticCommand(
            &getConfiguration()->diagnosticsManager, &command);
        diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    }

    return true;
}
