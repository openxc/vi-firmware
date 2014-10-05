#ifndef __DIAGNOSTIC_REQUEST_COMMAND_H__
#define __DIAGNOSTIC_REQUEST_COMMAND_H__

#include "openxc.pb.h"

namespace openxc {
namespace commands {

bool handleDiagnosticRequestCommand(openxc_ControlCommand* command);
bool validateDiagnosticRequest(openxc_VehicleMessage* message);

} // namespace commands
} // namespace openxc

#endif // __DIAGNOSTIC_REQUEST_COMMAND_H__
