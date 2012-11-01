#include "bitfield.h"
#include "canutil_lpc17xx.h"
#include "canutil.h"
#include "canwrite.h"
#include "log.h"
#include <stdbool.h>

void copyToMessageBuffer(uint64_t source, uint8_t* a, uint8_t* b) {
    for(int i = 0, j = 4; i < 3 && j < 8; i++, j++) {
        a[i] = nthByte(source, i);
        b[i] = nthByte(source, j);
    }
}

bool sendCanMessage(CanBus* bus, CanMessage request) {
    CAN_MSG_Type message;
    message.id =  request.id;
    message.len = 8;
    message.type = DATA_FRAME;
    message.format = STD_ID_FORMAT;
    copyToMessageBuffer(request.data, message.dataA, message.dataB);

    return CAN_SendMsg(CAN_CONTROLLER(bus), &message) == SUCCESS;
}
