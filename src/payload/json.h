#ifndef __JSON_H__
#define __JSON_H__

#include "openxc.pb.h"

namespace openxc {
namespace payload {
namespace json {

extern const char VERSION_COMMAND_NAME[];
extern const char DEVICE_ID_COMMAND_NAME[];
extern const char DIAGNOSTIC_COMMAND_NAME[];
extern const char PASSTHROUGH_COMMAND_NAME[];
extern const char ACCEPTANCE_FILTER_BYPASS_COMMAND_NAME[];
extern const char PAYLOAD_FORMAT_COMMAND_NAME[];
extern const char PREDEFINED_OBD2_REQUESTS_COMMAND_NAME[];

extern const char PAYLOAD_FORMAT_JSON_NAME[];
extern const char PAYLOAD_FORMAT_PROTOBUF_NAME[];

extern const char COMMAND_RESPONSE_FIELD_NAME[];
extern const char COMMAND_RESPONSE_MESSAGE_FIELD_NAME[];
extern const char COMMAND_RESPONSE_STATUS_FIELD_NAME[];

extern const char BUS_FIELD_NAME[];
extern const char ID_FIELD_NAME[];
extern const char DATA_FIELD_NAME[];
extern const char NAME_FIELD_NAME[];
extern const char VALUE_FIELD_NAME[];
extern const char EVENT_FIELD_NAME[];
extern const char FRAME_FORMAT_FIELD_NAME[];

extern const char FRAME_FORMAT_STANDARD_NAME[];
extern const char FRAME_FORMAT_EXTENDED_NAME[];

extern const char DIAGNOSTIC_MODE_FIELD_NAME[];
extern const char DIAGNOSTIC_PID_FIELD_NAME[];
extern const char DIAGNOSTIC_SUCCESS_FIELD_NAME[];
extern const char DIAGNOSTIC_NRC_FIELD_NAME[];
extern const char DIAGNOSTIC_PAYLOAD_FIELD_NAME[];
extern const char DIAGNOSTIC_VALUE_FIELD_NAME[];
extern const char MODEM_CONFIGURATION_COMMAND_NAME[];

/* Public: Deserialize an OpenXC message from a payload containing JSON.
 *
 * payload - The bytestream payload to parse a message from.
 * length -  The length of the payload.
 * message - An output parameter, the object to store the deserialized message.
 *
 * Returns the number of bytes parsed as JSON object from the payload, if any
 * was found.
 */
size_t deserialize(uint8_t payload[], size_t length, openxc_VehicleMessage* message);

/* Public: Serialize an OpenXC message as JSON and store in the payload.
 *
 * message - The message to serialize.
 * payload - The buffer to store the payload - must be allocated by the caller.
 * length -  The length of the payload buffer.
 *
 * Returns the number of bytes written to the payload. If the length is 0, an
 * error occurred while serializing.
 */
int serialize(openxc_VehicleMessage* message, uint8_t payload[], size_t length);

} // namespace json
} // namespace payload
} // namespace openxc

#endif // __JSON_H__
