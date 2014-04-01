#include "can/canwrite.h"
#include "canutil_pic32.h"
#include "util/log.h"

using openxc::util::log::debug;

/* Private: Copy message data to destination buffer as big endian, as opposed to the
 * micro's little endian (hence we can't just use memcpy).
 *
 * source - source data to copy.
 * destination - pointer to an array of uint8_t (must be size 8) to copy the
 *      data.
 */
void copyToMessageBuffer(uint64_t source, uint8_t* destination) {
    for(int i = 0; i < 8; i++) {
        destination[i] = ((uint8_t*)&source)[i];
    }
}

bool openxc::can::write::sendMessage(const CanBus* bus, const CanMessage* request) {
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
        memset(message->data, 0, 8);
        copyToMessageBuffer(request->data, message->data);

        // Mark message as ready to be processed
        CAN_CONTROLLER(bus)->updateChannel(CAN::CHANNEL0);
        CAN_CONTROLLER(bus)->flushTxChannel(CAN::CHANNEL0);
        return true;
    } else {
        debug("Unable to get TX message area");
    }
    return false;
}
