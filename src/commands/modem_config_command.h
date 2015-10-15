#ifndef __MODEM_CONFIGURATION_COMMAND_H__
#define __MODEM_CONFIGURATION_COMMAND_H__

#include "openxc.pb.h"

namespace openxc {
namespace commands {

bool validateModemConfigurationCommand(openxc_VehicleMessage* message);

bool handleModemConfigurationCommand(openxc_ControlCommand* command);

} // namespace commands
} // namespace openxc

#endif // __MODEM_CONFIGURATION_COMMAND_H__
