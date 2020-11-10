#ifndef __PAYLOAD_H__
#define __PAYLOAD_H__

#include "openxc.pb.h"
#include <stdint.h>

namespace openxc {
namespace payload {

/* Public: The available encoding formats for OpenXC payloads.
 */
typedef enum {
    JSON,
    PROTOBUF,
} PayloadFormat;

/* Public: Deserialize an OpenXC message from the given payload, using the given
 * format.
 *
 * The payload must be in one of the formats defined by the OpenXC message
 * format:
 *
 * https://github.com/openxc/openxc-message-format
 *
 * payload - The bytestream payload to parse a message from.
 * length -  The length of the payload.
 * format - The expected format of the message serialized in the payload.
 * message - An output parameter, the object to store the deserialized message.
 *
 * Returns the number of bytes read for a complete message from the payload, if
 * any where found.
 */
size_t deserialize(uint8_t payload[], size_t length, PayloadFormat format,
        openxc_VehicleMessage* message);

/* Public: Serialize an OpenXC message into a payload of bytes using the OpenXC
 * message format (https://github.com/openxc/openxc-message-format).
 *
 * message - The message to serialize.
 * payload - The buffer to store the payload - must be allocated by the caller.
 * length -  The length of the payload buffer.
 * format - The serialization format to use in the payload (e.g. JSON, protocol
 *      buffers, etc).
 *
 * Returns the number of bytes written to the payload. If the length is 0, an
 * error occurred while serializing.
 */
int serialize(openxc_VehicleMessage* message, uint8_t payload[], size_t length,
        PayloadFormat format);

/* Public: Helper functions to wrap values in an openxc_DynamicField
 */
openxc_DynamicField wrapNumber(float value);
openxc_DynamicField wrapString(const char* value);
openxc_DynamicField wrapBoolean(bool value);

} // namespace payload
} // namespace openxc

#endif // __PAYLOAD_H__
