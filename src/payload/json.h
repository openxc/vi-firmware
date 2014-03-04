#ifndef __JSON_H__
#define __JSON_H__

#include "openxc.pb.h"

namespace openxc {
namespace payload {
namespace json {

bool deserialize(uint8_t payload[], size_t length, openxc_VehicleMessage* message);

int serialize(openxc_VehicleMessage* message, uint8_t payload[], size_t length);

} // namespace json
} // namespace payload
} // namespace openxc

#endif // __JSON_H__
