#include "modem_config_command.h"
#include "config.h"
#include "util/log.h"
#include "../platform/pic32/nvm.h"
#include "platform/pic32/telit_he910_platforms.h"

namespace nvm = openxc::nvm;

using openxc::util::log::debug;
using openxc::config::getConfiguration;

bool openxc::commands::validateModemConfigurationCommand(openxc_VehicleMessage* message) {
    bool valid = false;
    //if(message->has_control_command) {
    if(message->type == openxc_VehicleMessage_Type_CONTROL_COMMAND) {
        openxc_ControlCommand* command = &message->control_command;
        //if(command->has_modem_configuration_command) {
        if(command->type == openxc_ControlCommand_Type_MODEM_CONFIGURATION) {
            valid = true;
        }
    }
    return valid;
}

bool openxc::commands::handleModemConfigurationCommand(openxc_ControlCommand* command) {
    #ifdef TELIT_HE910_SUPPORT
    bool status = false;
    if(command->has_modem_configuration_command) {
        openxc_ModemConfigurationCommand* modemConfigurationCommand =
          &command->modem_configuration_command;
        // extract configs for each has_<sub-messages>
        if(modemConfigurationCommand->has_serverConnectSettings) {
            openxc_ServerConnectSettings* serverConnectSettings = 
              &modemConfigurationCommand->serverConnectSettings;
            if(serverConnectSettings->has_host) {
                strcpy(getConfiguration()->telit->config.serverConnectSettings.host, 
                  serverConnectSettings->host);
                status = true;
                debug("Set server address to %s", 
                  getConfiguration()->telit->config.serverConnectSettings.host);
                status = true;
            }
            if(serverConnectSettings->has_port) {
                getConfiguration()->telit->config.serverConnectSettings.port = 
                  serverConnectSettings->port;
                debug("Set server port to %u", 
                  getConfiguration()->telit->config.serverConnectSettings.port);
            }
        }
    }

    if(status) {
        // save new settings to NVM
        nvm::store();
    }

    sendCommandResponse(openxc_ControlCommand_Type_MODEM_CONFIGURATION, status);

    return status;

    #else 

    return false;

    #endif
}
