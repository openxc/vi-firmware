#ifndef __PAYLOAD_FORMAT_COMMAND_H__
#define __PAYLOAD_FORMAT_COMMAND_H__

#include "openxc.pb.h"

namespace openxc {
namespace commands {

bool validatePayloadFormatCommand(openxc_VehicleMessage* message);

bool handlePayloadFormatCommand(openxc_ControlCommand* command);

} // namespace commands
} // namespace openxc

#endif // __PAYLOAD_FORMAT_COMMAND_H__
