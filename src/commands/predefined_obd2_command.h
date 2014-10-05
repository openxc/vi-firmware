#ifndef __PREDEFINED_OBD2_COMMAND_H__
#define __PREDEFINED_OBD2_COMMAND_H__

#include "openxc.pb.h"

namespace openxc {
namespace commands {

bool validatePredefinedObd2RequestsCommand(openxc_VehicleMessage* message);

bool handlePredefinedObd2RequestsCommand(openxc_ControlCommand* command);

} // namespace commands
} // namespace openxc

#endif // __PREDEFINED_OBD2_COMMAND_H__
