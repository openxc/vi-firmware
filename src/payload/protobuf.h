#ifndef __PROTOBUF_H__
#define __PROTOBUF_H__

#include "openxc.pb.h"

namespace openxc {
namespace payload {
namespace protobuf {

/* Public: Deserialize an OpenXC message from a payload containing a Protocol
 * Buffer.
 *
 * payload - The bytestream payload to parse a message from.
 * length -  The length of the payload.
 * message - An output parameter, the object to store the deserialized message.
 *
 * Returns the number of bytes parsed as a protobuf from the payload, if any was
 * found.
 */
size_t deserialize(uint8_t payload[], size_t length, openxc_VehicleMessage* message);

/* Public: Serialize an OpenXC message as a Protocol Buffer and store in the
 * payload.
 *
 * message - The message to serialize.
 * payload - The buffer to store the payload - must be allocated by the caller.
 * length -  The length of the payload buffer.
 *
 * Returns the number of bytes written to the payload. If the length is 0, an
 * error occurred while serializing.
 */
int serialize(openxc_VehicleMessage* message, uint8_t payload[], size_t length);

} // namespace protobuf
} // namespace payload
} // namespace openxc

#endif // __PROTOBUF_H__
