#ifndef __RTC_CONFIGURATION_COMMAND_H__
#define __RTC_CONFIGURATION_COMMAND_H__

#include "openxc.pb.h"

namespace openxc {
namespace commands {

bool validateRTCConfigurationCommand(openxc_VehicleMessage* message);

bool handleRTCConfigurationCommand(openxc_ControlCommand* command);

} // namespace commands
} // namespace openxc

#endif // __RTC_CONFIGURATION_COMMAND_H__
