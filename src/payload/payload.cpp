#include "payload.h"
#include "payload/json.h"
#include "payload/protobuf.h"
#include "payload/messagepack.h"
#include "util/log.h"

namespace payload = openxc::payload;

using openxc::util::log::debug;

openxc_DynamicField openxc::payload::wrapNumber(float value) {
    openxc_DynamicField sabot = openxc_DynamicField();	// Zero Fill
    //sabot.has_type = true;
    sabot.type = openxc_DynamicField_Type_NUM;
    //sabot.has_numeric_value = true;
    sabot.numeric_value = value;
    return sabot;
}

openxc_DynamicField openxc::payload::wrapString(const char* value) {
    openxc_DynamicField sabot = openxc_DynamicField();	// Zero Fill
    //sabot.has_type = true;
    sabot.type = openxc_DynamicField_Type_STRING;
    //sabot.has_string_value = true;
    strcpy(sabot.string_value, value);
    return sabot;
}

openxc_DynamicField openxc::payload::wrapBoolean(bool value) {
    openxc_DynamicField sabot = openxc_DynamicField();	// Zero Fill
    //sabot.has_type = true;
    sabot.type = openxc_DynamicField_Type_BOOL;
    //sabot.has_boolean_value = true;
    sabot.boolean_value = value;
    return sabot;
}

size_t openxc::payload::deserialize(uint8_t payload[], size_t length,
        PayloadFormat format, openxc_VehicleMessage* message) {
    size_t bytesRead = 0;
    if(format == PayloadFormat::JSON) {
        bytesRead = payload::json::deserialize(payload, length, message);
    } else if(format == PayloadFormat::PROTOBUF) {
        bytesRead = payload::protobuf::deserialize(payload, length, message);
    } else if(format == PayloadFormat::MESSAGEPACK){
        bytesRead = payload::messagepack::deserialize(payload, length, message);
    } else {
        debug("Invalid payload format: %d", format);
    }
    return bytesRead;
}

int openxc::payload::serialize(openxc_VehicleMessage* message,
        uint8_t payload[], size_t length, PayloadFormat format) {
    int serializedLength = 0;
    if(format == PayloadFormat::JSON) {
        serializedLength = payload::json::serialize(message, payload, length);
    } else if(format == PayloadFormat::PROTOBUF) {
        serializedLength = payload::protobuf::serialize(message, payload, length);
    } else if(format == PayloadFormat::MESSAGEPACK) {
        serializedLength = payload::messagepack::serialize(message, payload, length);
    } else {
        debug("Invalid payload format: %d", format);
    }
    return serializedLength;
}
