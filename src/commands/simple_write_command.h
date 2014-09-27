#ifndef __SIMPLE_WRITE_COMMAND_H__
#define __SIMPLE_WRITE_COMMAND_H__

#include "commands/commands.h"

namespace openxc {
namespace commands {

bool handleTranslated(openxc_VehicleMessage* message);

bool validateTranslated(openxc_VehicleMessage* message);

} // namespace commands
} // namespace openxc

#endif // __SIMPLE_WRITE_COMMAND_H__
