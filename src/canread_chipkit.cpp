#ifdef __CHIPKIT__

#include "canread.h"

/*
 * Check to see if a packet has been received. If so, read the packet and print
 * the packet payload to the serial monitor.
 */
void receiveCan(CanBus* bus) {

    if(bus->messageReceived == false) {
        // The flag is updated by the CAN ISR.
        return;
    }
    ++receivedMessages;

    CanMessage message = receiveCanMessage(bus->bus);
    decodeCanMessage(message.id, message.data);
}

CanMessage receiveCanMessage(CanBus* bus) {
    CAN::RxMessageBuffer* message;

    CAN::RxMessageBuffer* message = bus->bus->getRxMessage(CAN::CHANNEL1);

    /* Call the CAN::updateChannel() function to let the CAN module know that
     * the message processing is done. Enable the event so that the CAN module
     * generates an interrupt when the event occurs.*/
    bus->bus->updateChannel(CAN::CHANNEL1);
    bus->bus->enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);

    bus->messageReceived = false;

    return {message->msgSID.SID, message->data};
}

/* Called by the Interrupt Service Routine whenever an event we registered for
 * occurs - this is where we wake up and decide to process a message. */
void handleCan1Interrupt() {
    if((can1.getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(can1.getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            can1.enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);
            getCanBuses()[0].messageReceived = true;
        }
    }
}

void handleCan2Interrupt() {
    if((can2.getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(can2.getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            can2.enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);
            getCanBuses()[1].messageReceived = true;
        }
    }
}

#endif // __CHIPKIT__
