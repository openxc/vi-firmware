#ifndef __PAYLOAD_H__
#define __PAYLOAD_H__

#include "openxc.pb.h"
#include <stdint.h>

namespace openxc {
namespace payload {

typedef enum {
    JSON,
    PROTOBUF,
} PayloadFormat;

bool deserialize(uint8_t payload[], size_t length, openxc_VehicleMessage* message,
        PayloadFormat format);

int serialize(openxc_VehicleMessage* message, uint8_t payload[], size_t length,
        PayloadFormat format);

} // namespace payload
} // namespace openxc

#endif // __PAYLOAD_H__
