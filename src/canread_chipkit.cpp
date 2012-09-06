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
#endif // __CHIPKIT__
