#ifndef __SIMPLE_WRITE_COMMAND_H__
#define __SIMPLE_WRITE_COMMAND_H__

#include "openxc.pb.h"

namespace openxc {
namespace commands {

bool handleSimple(openxc_VehicleMessage* message);

bool validateSimple(openxc_VehicleMessage* message);

} // namespace commands
} // namespace openxc

#endif // __SIMPLE_WRITE_COMMAND_H__
