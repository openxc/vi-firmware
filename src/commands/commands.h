#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdint.h>
#include <stdlib.h>
#include "openxc.pb.h"

namespace openxc {
namespace commands {

#define CONTROL_COMMAND_REQUEST_ID 0x83

/* Public: Handle a new command received on an I/O interface.
 *
 * This as an implementation of
 * openxc::util::bytebuffer::IncomingMessageCallback so it can be directly
 * passed to any module using that interface (e.g. both USB and UART). It will
 * attempt to deserialize a command from the payload using the currently
 * configured payload format and perform the desired action, if recognized and
 * allowed.
 *
 * The currently suported commands are:
 *
 *   - Raw CAN message writes
 *   - Translated CAN signal writes
 *   - Control commands:
 *       - Diagnostic message requests
 *       - Firmware version query
 *       - VI device ID query
 *
 * The complete definition for all of the command is in the OpenXC Message
 * Format (https://github.com/openxc/openxc-message-format).
 *
 * Returns the number of bytes read from the payload for a complete message, if
 * any was found.
 */
size_t handleIncomingMessage(uint8_t payload[], size_t payloadLength);

/* Public: Validate the data in an OpenXC message;
 *
 * For example, confirms that a RawMessage typed message has both an id an data
 * field.
 *
 * Returns true if the message is valid.
 */
bool validate(openxc_VehicleMessage* message);

} // namespace commands
} // namespace openxc

#endif // __COMMANDS_H__
