#ifndef __JSON_H__
#define __JSON_H__

#include "openxc.pb.h"

namespace openxc {
namespace payload {
namespace json {

extern const char VERSION_COMMAND_NAME[];
extern const char DEVICE_ID_COMMAND_NAME[];
extern const char DIAGNOSTIC_COMMAND_NAME[];

extern const char BUS_FIELD_NAME[];
extern const char ID_FIELD_NAME[];
extern const char DATA_FIELD_NAME[];
extern const char NAME_FIELD_NAME[];
extern const char VALUE_FIELD_NAME[];
extern const char EVENT_FIELD_NAME[];

extern const char DIAGNOSTIC_MODE_FIELD_NAME[];
extern const char DIAGNOSTIC_PID_FIELD_NAME[];
extern const char DIAGNOSTIC_SUCCESS_FIELD_NAME[];
extern const char DIAGNOSTIC_NRC_FIELD_NAME[];
extern const char DIAGNOSTIC_PAYLOAD_FIELD_NAME[];
extern const char DIAGNOSTIC_VALUE_FIELD_NAME[];

bool deserialize(uint8_t payload[], size_t length, openxc_VehicleMessage* message);

int serialize(openxc_VehicleMessage* message, uint8_t payload[], size_t length);

} // namespace json
} // namespace payload
} // namespace openxc

#endif // __JSON_H__
