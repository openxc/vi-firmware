#include "can/canwrite.h"
#include "canutil_pic32.h"
#include "util/log.h"

using openxc::util::log::debug;

bool openxc::can::write::sendMessage(CanBus* bus, CanMessage* request) {
    CAN::TxMessageBuffer* message = CAN_CONTROLLER(bus)->getTxMessageBuffer(
            CAN::CHANNEL0);
    if (message != NULL) {
        message->messageWord[0] = 0;
        message->messageWord[1] = 0;
        message->messageWord[2] = 0;
        message->messageWord[3] = 0;

        if(request->format == CanMessageFormat::STANDARD) {
            message->msgEID.IDE = 0;
            message->msgSID.SID = request->id;
        } else {
            message->msgEID.IDE = 1;
            message->msgSID.SID = request->id >> 18;
            // This will truncate the front 11 bits, which is fine because we
            // stored them in the SID field
            message->msgEID.EID = request->id;
        }
        message->msgEID.DLC = request->length;
        memcpy(message->data, request->data, request->length);

        // Mark message as ready to be processed
        CAN_CONTROLLER(bus)->updateChannel(CAN::CHANNEL0);
        CAN_CONTROLLER(bus)->flushTxChannel(CAN::CHANNEL0);
        return true;
    } else {
        debug("Unable to get TX message area");
    }
    return false;
}
