#ifndef __PASSTHROUGH_COMMAND_H__
#define __PASSTHROUGH_COMMAND_H__

#include "openxc.pb.h"

namespace openxc {
namespace commands {

bool validatePassthroughRequest(openxc_VehicleMessage* message);

bool handlePassthroughModeCommand(openxc_ControlCommand* command);

} // namespace commands
} // namespace openxc

#endif // __PASSTHROUGH_COMMAND_H__
