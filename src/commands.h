#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdint.h>

namespace openxc {
namespace commands {

#define VERSION_CONTROL_COMMAND 0x80
#define DEVICE_ID_CONTROL_COMMAND 0x82
#define DIAGNOSTIC_REQUEST_CONTROL_COMMAND 0x83

bool handleCommand(uint8_t request, uint8_t payload[], int payloadLength);

} // namespace commands
} // namespace openxc

#endif // __COMMANDS_H__
