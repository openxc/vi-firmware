#include <bitfield/bitfield.h>
#include <bitfield/8byte.h>
#include "can/canutil.h"
#include "can/canwrite.h"
#include "util/log.h"

bool openxc::can::write::sendMessage(CanBus* bus, CanMessage request) {
    CAN_MSG_Type message;
    message.id =  request.id;
    message.len = 8;
    message.type = DATA_FRAME;
    message.format = STD_ID_FORMAT;
    memcpy(message.dataA, &request.data[3], 4);
    memcpy(message.dataB, request.data, 4);

    return CAN_SendMsg(CAN_CONTROLLER(bus), &message) == SUCCESS;
}
