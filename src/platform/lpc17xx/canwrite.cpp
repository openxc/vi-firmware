#include <bitfield/bitfield.h>
#include <bitfield/8byte.h>
#include "can/canutil.h"
#include "canutil_lpc17xx.h"
#include "can/canwrite.h"
#include "util/log.h"

void copyToMessageBuffer(uint64_t source, uint8_t* a, uint8_t* b) {
    for(int i = 0, j = 4; i < 4 && j < 8; i++, j++) {
        a[3 - i] = eightbyte_get_byte(source, j, false);
        b[3 - i] = eightbyte_get_byte(source, i, false);
    }
}

bool openxc::can::write::sendMessage(const CanBus* bus, const CanMessage* request) {
    CAN_MSG_Type message;
    message.id =  request->id;
    message.len = 8;
    message.type = DATA_FRAME;
    message.format = STD_ID_FORMAT;
    copyToMessageBuffer(request->data, message.dataA, message.dataB);

    return CAN_SendMsg(CAN_CONTROLLER(bus), &message) == SUCCESS;
}
