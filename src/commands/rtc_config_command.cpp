#include "rtc_config_command.h"
#include "config.h"
#include "util/log.h"
#include "interface/fs.h"
#include "fs_platforms.h"

namespace fs = openxc::interface::fs;

bool openxc::commands::validateRTCConfigurationCommand(openxc_VehicleMessage* message) {
    bool valid = false;
    if(message->has_control_command) {
        openxc_ControlCommand* command = &message->control_command;
        if(command->has_rtc_configuration_command) {
            valid = true;
        }
    }
    return valid;
}

bool openxc::commands::handleRTCConfigurationCommand(openxc_ControlCommand* command) {
	
	bool status = false;
	
    #ifdef FS_SUPPORT
    if(command->has_rtc_configuration_command) {
        openxc_RTCConfigurationCommand* rtcConfigurationCommand =
          &command->rtc_configuration_command;

		if(rtcConfigurationCommand->has_unix_time){
			uint32_t * new_unix_time =
			  &rtcConfigurationCommand->unix_time;
			
			status = fs::setRTC(new_unix_time);
				
		}
    }
    #else 
	status = false;
    #endif
	sendCommandResponse(openxc_ControlCommand_Type_RTC_CONFIGURATION, status);
	
    return status;
}
