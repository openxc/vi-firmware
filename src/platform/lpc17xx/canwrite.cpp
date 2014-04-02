#include <bitfield/bitfield.h>
#include <bitfield/8byte.h>
#include "can/canutil.h"
#include "canutil_lpc17xx.h"
#include "can/canwrite.h"

bool openxc::can::write::sendMessage(const CanBus* bus, const CanMessage* request) {
    CAN_MSG_Type message;
    message.id =  request->id;
    message.len = request->length;
    message.type = DATA_FRAME;
    if(request->format == CanMessageFormat::STANDARD) {
        message.format = STD_ID_FORMAT;
    } else {
        message.format = EXT_ID_FORMAT;
    }
    memcpy(message.dataA, request->data, 4);
    memcpy(message.dataB, &(request->data[4]), 4);

    return CAN_SendMsg(CAN_CONTROLLER(bus), &message) == SUCCESS;
}
