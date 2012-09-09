#ifdef __LPC17XX__

#include "canread.h"
#include "signals.h"

extern "C" {

void CAN_IRQHandler() {
    if((CAN_IntGetStatus(LPC_CAN1) & 0x01) == 1) {
        getCanBuses()[0].messageReceived = true;
    } else if((CAN_IntGetStatus(LPC_CAN2) & 0x01) == 1) {
        getCanBuses()[1].messageReceived = true;
    }
}

}

CanMessage receiveCanMessage(CanBus* bus) {
    CAN_MSG_Type message;
    CAN_ReceiveMsg(bus->controller, &message);

    CanMessage result = {message.id, 0};
    result.data = message.dataA[0];
    result.data |= (message.dataA[1] << 8);
    result.data |= (message.dataA[2] << 16);
    result.data |= (message.dataA[3] << 24);
    result.data |= (((uint64_t)message.dataB[0]) << 32);
    result.data |= (((uint64_t)message.dataB[1]) << 40);
    result.data |= (((uint64_t)message.dataB[2]) << 48);
    result.data |= (((uint64_t)message.dataB[3]) << 56);

    return result;
}

#endif // __LPC17XX__
