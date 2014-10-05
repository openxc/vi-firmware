#ifndef __AF_BYPASS_COMMAND_H__
#define __AF_BYPASS_COMMAND_H__

#include "openxc.pb.h"

namespace openxc {
namespace commands {

bool validateFilterBypassCommand(openxc_VehicleMessage* message);

bool handleFilterBypassCommand(openxc_ControlCommand* command);

} // namespace commands
} // namespace openxc

#endif // __AF_BYPASS_COMMAND_H__
