#ifndef __CAN_MESSAGE_WRITE_COMMAND_H__
#define __CAN_MESSAGE_WRITE_COMMAND_H__

#include "openxc.pb.h"
#include "interface/interface.h"

namespace openxc {
namespace commands {

bool handleCan(openxc_VehicleMessage* message,
        openxc::interface::InterfaceDescriptor* sourceInterfaceDescriptor);

bool validateCan(openxc_VehicleMessage* message);

} // namespace commands
} // namespace openxc

#endif // __CAN_MESSAGE_WRITE_COMMAND_H__
