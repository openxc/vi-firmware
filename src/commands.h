#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdint.h>
#include <stdlib.h>

namespace openxc {
namespace commands {

#define VERSION_CONTROL_COMMAND 0x80
#define DEVICE_ID_CONTROL_COMMAND 0x82
#define DIAGNOSTIC_REQUEST_CONTROL_COMMAND 0x83

typedef enum {
    VERSION = 0x80,
    DEVICE_ID = 0x82,
    COMPLEX_COMMAND = 0x83
} Command;

typedef bool (*IncomingMessageCallback)(uint8_t*, size_t);

bool handleIncomingMessage(uint8_t payload[], size_t payloadLength);

bool handleControlCommand(Command command, uint8_t payload[],
        size_t payloadLength);

} // namespace commands
} // namespace openxc

#endif // __COMMANDS_H__
