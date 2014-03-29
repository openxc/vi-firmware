#ifndef __PROTOBUF_H__
#define __PROTOBUF_H__

#include "openxc.pb.h"

namespace openxc {
namespace payload {
namespace protobuf {

bool deserialize(uint8_t payload[], size_t length, openxc_VehicleMessage* message);

int serialize(openxc_VehicleMessage* message, uint8_t payload[], size_t length);

} // namespace protobuf
} // namespace payload
} // namespace openxc

#endif // __PROTOBUF_H__
