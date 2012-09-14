#include "canread.h"
#include "signals.h"

CanMessage receiveCanMessage(CanBus* bus) {
    CAN::RxMessageBuffer* message = bus->controller->getRxMessage(CAN::CHANNEL1);

    CanMessage result = {message->msgSID.SID, 0};
    result.data = message->data[0];
    result.data |= (((uint64_t)message->data[1]) << 8);
    result.data |= (((uint64_t)message->data[2]) << 16);
    result.data |= (((uint64_t)message->data[3]) << 24);
    result.data |= (((uint64_t)message->data[4]) << 32);
    result.data |= (((uint64_t)message->data[5]) << 40);
    result.data |= (((uint64_t)message->data[6]) << 48);
    result.data |= (((uint64_t)message->data[7]) << 56);
    return result;
}

void handleCanInterrupt(CanBus* bus) {
    if((bus->controller->getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(bus->controller->getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            bus->controller->enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);

            QUEUE_PUSH(CanMessage, &bus->receiveQueue, receiveCanMessage(bus));

            /* Call the CAN::updateChannel() function to let the CAN module know that
             * the message processing is done. Enable the event so that the CAN module
             * generates an interrupt when the event occurs.*/
            bus->controller->updateChannel(CAN::CHANNEL1);
            bus->controller->enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, true);
        }
    }
}

/* Called by the Interrupt Service Routine whenever an event we registered for
 * occurs - this is where we wake up and decide to process a message. */
void handleCan1Interrupt() {
    handleCanInterrupt(&getCanBuses()[0]);
}

void handleCan2Interrupt() {
    handleCanInterrupt(&getCanBuses()[1]);
}
