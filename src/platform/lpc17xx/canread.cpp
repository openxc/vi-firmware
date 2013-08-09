#include "can/canread.h"
#include "signals.h"
#include "util/log.h"

using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;

CanMessage receiveCanMessage(CanBus* bus) {
    CAN_MSG_Type message;
    CAN_ReceiveMsg(CAN_CONTROLLER(bus), &message);

    CanMessage result = {bus, message.id, 0};
    result.data = message.dataA[0];
    result.data |= (((uint64_t)message.dataA[1]) << 8);
    result.data |= (((uint64_t)message.dataA[2]) << 16);
    result.data |= (((uint64_t)message.dataA[3]) << 24);
    result.data |= (((uint64_t)message.dataB[0]) << 32);
    result.data |= (((uint64_t)message.dataB[1]) << 40);
    result.data |= (((uint64_t)message.dataB[2]) << 48);
    result.data |= (((uint64_t)message.dataB[3]) << 56);

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
