#include "can/canread.h"
#include "signals.h"
#include "util/log.h"

using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;

CanMessage receiveCanMessage(CanBus* bus) {
    CAN_MSG_Type message;
    CAN_ReceiveMsg(CAN_CONTROLLER(bus), &message);

    CanMessage result = {message.id};
    // TODO is this backwards?
    memcpy(result.data, message->dataB, 4);
    memcpy(result.data[3], message->dataA, 4);
    return result;
}

#ifndef CAN_EMULATOR

extern "C" {

void CAN_IRQHandler() {
    for(int i = 0; i < getCanBusCount(); i++) {
        CanBus* bus = &getCanBuses()[i];
        if((CAN_IntGetStatus(CAN_CONTROLLER(bus)) & 0x01) == 1) {
            CanMessage message = receiveCanMessage(bus);
            if(!QUEUE_PUSH(CanMessage, &bus->receiveQueue, message)) {
                // An exception to the "don't leave commented out code" rule,
                // this log statement is useful for debugging performance issues
                // but if left enabled all of the time, it can can slown down
                // the interrupt handler so much that it locks up the device in
                // permanent interrupt handling land.
                //
                // debug("Dropped CAN message with ID 0x%02x -- queue is full",
                // message.id);
                ++bus->messagesDropped;
            }
        }
    }
}

}

#endif // CAN_EMULATOR
