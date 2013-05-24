#include "util/bitfield.h"
#include "can/canutil.h"
#include "can/canwrite.h"
#include "util/log.h"

using openxc::util::bitfield::nthByte;

void copyToMessageBuffer(uint64_t source, uint8_t* a, uint8_t* b) {
    for(int i = 0, j = 4; i < 4 && j < 8; i++, j++) {
        a[3 - i] = nthByte(source, j);
        b[3 - i] = nthByte(source, i);
    }
}

bool openxc::can::write::sendMessage(CanBus* bus, CanMessage request) {
    CAN_MSG_Type message;
    message.id =  request.id;
    message.len = 8;
    message.type = DATA_FRAME;
    message.format = STD_ID_FORMAT;
    copyToMessageBuffer(request.data, message.dataA, message.dataB);

    return CAN_SendMsg(CAN_CONTROLLER(bus), &message) == SUCCESS;
}
