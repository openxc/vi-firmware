#include "protobuf.h"

#include <util/log.h>
#include "pb_encode.h"
#include "pb_decode.h"

using openxc::util::log::debug;

size_t openxc::payload::protobuf::deserialize(uint8_t payload[], size_t length,
        openxc_VehicleMessage* message) {
    pb_istream_t stream = pb_istream_from_buffer(payload, length);
    if(!pb_decode_delimited(&stream, openxc_VehicleMessage_fields, message)) {
        debug("Protobuf decoding failed with %s", PB_GET_ERROR(&stream));
        return 0;
    }
    return length - stream.bytes_left;
}

int openxc::payload::protobuf::serialize(openxc_VehicleMessage* message,
        uint8_t payload[], size_t length) {
    if(message == NULL) {
        debug("Message object is NULL");
        return 0;
    }

    pb_ostream_t stream = pb_ostream_from_buffer(payload, length);
    if(!pb_encode_delimited(&stream, openxc_VehicleMessage_fields,
            message)) {
        debug("Error encoding protobuf: %s", PB_GET_ERROR(&stream));
    }
    return stream.bytes_written;
}
