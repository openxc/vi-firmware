#include <payload/payload.h>
#include <payload/json.h>
#include <payload/protobuf.h>
#include <util/log.h>

namespace payload = openxc::payload;

using openxc::util::log::debug;

bool openxc::payload::deserialize(uint8_t payload[], size_t length,
        openxc_VehicleMessage* message, PayloadFormat format) {
    bool status = false;
    if(format == PayloadFormat::JSON) {
        status = payload::json::deserialize(payload, length, message);
    } else if(format == PayloadFormat::PROTOBUF) {
        status = payload::protobuf::deserialize(payload, length, message);
    } else {
        debug("Invalid payload format: %d", format);
    }
    return status;
}

int openxc::payload::serialize(openxc_VehicleMessage* message,
        uint8_t payload[], size_t length, PayloadFormat format) {
    int serializedLength = 0;
    if(format == PayloadFormat::JSON) {
        serializedLength = payload::json::serialize(message, payload, length);
    } else if(format == PayloadFormat::PROTOBUF) {
        serializedLength = payload::protobuf::serialize(message, payload, length);
    } else {
        debug("Invalid payload format: %d", format);
    }
    return serializedLength;
}
