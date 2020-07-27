#include "payload.h"
#include "payload/json.h"
#include "payload/protobuf.h"
#include "payload/messagepack.h"
#include "util/log.h"
#include <stdio.h>

namespace payload = openxc::payload;

using openxc::util::log::debug;

openxc_DynamicField openxc::payload::wrapNumber(float value) {
    openxc_DynamicField sabot = openxc_DynamicField();	// Zero Fill
    sabot.type = openxc_DynamicField_Type_NUM;
    sabot.numeric_value = value;
    return sabot;
}

openxc_DynamicField openxc::payload::wrapString(const char* value) {
    openxc_DynamicField sabot = openxc_DynamicField();	// Zero Fill
    sabot.type = openxc_DynamicField_Type_STRING;
    strcpy(sabot.string_value, value);
    return sabot;
}

openxc_DynamicField openxc::payload::wrapBoolean(bool value) {
    openxc_DynamicField sabot = openxc_DynamicField();	// Zero Fill
    sabot.type = openxc_DynamicField_Type_BOOL;
    sabot.boolean_value = value;
    return sabot;
}

// Diagnostically print out the hex values in the payload
void dumpPayload(unsigned char *payload, size_t length) {
    int finished = 0;
    size_t offset = 0;
    const size_t MAX = 12;
    while(!finished) {
        char buf[26];
        buf[0] = 0;
        size_t l = length-offset;
        if (l > MAX) 
            l = MAX;
        for(size_t i=0; i<l; i++) {
            buf[i*2]= ((payload[i+offset]>>4) > 9) ? (payload[i+offset]>>4) + 'A' - 10 : (payload[i+offset]>>4) + '0';
            buf[i*2+1]=((payload[i+offset]&0xf) > 9) ? (payload[i+offset]&0x0f) + 'A' - 10 : (payload[i+offset]&0xf) + '0';
            buf[i*2+2]=0;        
        }
        debug(buf);
        offset += MAX;
        if (offset >= length) finished = 1;
    }
}

void dumpNum(int value) {
    char buf[10];
    sprintf(buf,"%d",value);
    debug(buf);
}

size_t openxc::payload::deserialize(uint8_t payload[], size_t length,
        PayloadFormat format, openxc_VehicleMessage* message) {
    size_t bytesRead = 0;
    if(format == PayloadFormat::JSON) {
        bytesRead = payload::json::deserialize(payload, length, message);
    } else if(format == PayloadFormat::PROTOBUF) {
        bytesRead = payload::protobuf::deserialize(payload, length, message);
        debug("deserialize protobuf");
        dumpNum(bytesRead);
        dumpPayload(payload, bytesRead);
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
