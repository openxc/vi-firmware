#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdint.h>
#include <stdlib.h>
#include "openxc.pb.h"

namespace openxc {
namespace commands {

#define VERSION_CONTROL_COMMAND 0x80
#define DEVICE_ID_CONTROL_COMMAND 0x82
#define DIAGNOSTIC_REQUEST_CONTROL_COMMAND 0x83

/* Public: USB control message identifiers for control commands.
 *
 * These map from USB control message IDs to commands that can also be send in
 * the normal data stream.
 */
typedef enum {
    VERSION = 0x80,
    DEVICE_ID = 0x82,
    COMPLEX_COMMAND = 0x83
} UsbControlCommand;

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
 * Returns true if the message was handled without error.
 */
bool handleIncomingMessage(uint8_t payload[], size_t payloadLength);

/* Public: Handle an incoming USB control message.
 *
 * This maps USB control messages with a recognized ID to control commands
 * supported by handleIncomingMessage. If you are attached to a VI via USB, you
 * can use the control EP0 for these commands instead of publishing them into
 * the normal bulk data stream.
 *
 * Returns true if the control command was recognized.
 */
bool handleControlCommand(UsbControlCommand command, uint8_t payload[],
        size_t payloadLength);

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
