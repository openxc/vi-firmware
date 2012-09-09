#ifdef __CHIPKIT__

#include "canread.h"

CanMessage receiveCanMessage(CanBus* bus) {
    CAN::RxMessageBuffer* message;

    CAN::RxMessageBuffer* message = bus->controller->getRxMessage(CAN::CHANNEL1);

    /* Call the CAN::updateChannel() function to let the CAN module know that
     * the message processing is done. Enable the event so that the CAN module
     * generates an interrupt when the event occurs.*/
    bus->controller->updateChannel(CAN::CHANNEL1);
    bus->controller->enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);

    return {message->msgSID.SID, message->data};
}

void handleCanInterrupt(CanBus* bus) {
    if((bus->controller.getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(bus->controller.getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            bus->controller.enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);

            CanMessage message = receiveCanMessage(bus);
            QUEUE_PUSH(CanMessage, &bus->receiveQueue, message);
        }
    }
}

/* Called by the Interrupt Service Routine whenever an event we registered for
 * occurs - this is where we wake up and decide to process a message. */
void handleCan1Interrupt() {
    handleCanInterrupt(getCanBuses()[0]);
}

void handleCan2Interrupt() {
    handleCanInterrupt(getCanBuses()[0]);
}


#endif // __CHIPKIT__
