/*
 *
 *  Derived from CanDemo.pde, example code that came with the
 *  chipKIT Network Shield libraries.
 */

#include "WProgram.h"
#include "chipKITCAN.h"
#include "chipKITUSBDevice.h"
#include "bitfield.h"
#include "canutil.h"
#include "usbutil.h"

/* Network Node Addresses */
#define CAN_1_ADDRESS 0x101
#define CAN_2_ADDRESS 0x102

#define SYS_FREQ (80000000L)
#define CAN_BUS_1_SPEED 500000
#define CAN_BUS_2_SPEED 500000


CAN can1(CAN::CAN1);
CAN can2(CAN::CAN2);

/* CAN Message Buffers */
uint8_t can1MessageArea[2 * 8 * 16];
uint8_t can2MessageArea[2 * 8 * 16];

/* These are used as event flags by the interrupt service routines. */
static volatile bool isCan1MessageReceived = false;
static volatile bool isCan2MessageReceived = false;

/* Forward declarations */

void initializeCan(uint32_t);
void receiveCan(CAN*, volatile bool*);

void setup() {
    Serial.begin(115200);

    initializeUsb();

    initializeCan(&can1, CAN_1_ADDRESS, CAN_BUS_1_SPEED, can1MessageArea);
    initializeCan(&can2, CAN_2_ADDRESS, CAN_BUS_2_SPEED, can2MessageArea);

    can1.attachInterrupt(handleCan1Interrupt);
    can2.attachInterrupt(handleCan2Interrupt);
}

void loop() {
    receiveCan(&can1, &isCan1MessageReceived);
    receiveCan(&can2, &isCan2MessageReceived);
}

/* Initialize the CAN controller. See inline comments for description of the
 * process.
 */
void initializeCan(CAN* bus, int address, int speed, uint8_t* messageArea) {
    Serial.print("Initializing CAN bus at ");
    Serial.println(address, DEC);
    CAN::BIT_CONFIG canBitConfig;

    /* Switch the CAN module ON and switch it to Configuration mode. Wait till
     * the switch is complete */
    bus->enableModule(true);
    bus->setOperatingMode(CAN::CONFIGURATION);
    while(bus->getOperatingMode() != CAN::CONFIGURATION);

    /* Configure the CAN Module Clock. The CAN::BIT_CONFIG data structure
     * is used for this purpose. The propagation, phase segment 1 and phase
     * segment 2 are configured to have 3TQ. The CANSetSpeed() function sets the
     * baud. */
    canBitConfig.phaseSeg2Tq            = CAN::BIT_3TQ;
    canBitConfig.phaseSeg1Tq            = CAN::BIT_3TQ;
    canBitConfig.propagationSegTq       = CAN::BIT_3TQ;
    canBitConfig.phaseSeg2TimeSelect    = CAN::TRUE;
    canBitConfig.sample3Time            = CAN::TRUE;
    canBitConfig.syncJumpWidth          = CAN::BIT_2TQ;
    bus->setSpeed(&canBitConfig, SYS_FREQ, speed);

    /* Assign the buffer area to the CAN module. */
    /* Note the size of each Channel area. It is 2 (Channels) * 8 (Messages
     * Buffers) 16 (bytes/per message buffer) bytes. Each CAN module should have
     * its own message area. */
    bus->assignMemoryBuffer(messageArea, 2 * 8 * 16);

    /* Configure channel 1 for RX and size of 8 message buffers and receive the
     * full message.
     */
    bus->configureChannelForRx(CAN::CHANNEL1, 8, CAN::RX_FULL_RECEIVE);

    int filterMaskCount;
    CanFilterMask* filterMasks = initializeFilterMasks(address,
            &filterMaskCount);
    int filterCount;
    CanFilter* filters = initializeFilters(address, &filterCount);
    configureFilters(bus, filterMasks, filterMaskCount, filters, filterCount);

    /* Enable interrupt and events. Enable the receive channel not empty
     * event (channel event) and the receive channel event (module event). The
     * interrrupt peripheral library is used to enable the CAN interrupt to the
     * CPU. */
    bus->enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);
    bus->enableModuleEvent(CAN::RX_EVENT, true);

    bus->setOperatingMode(CAN::LISTEN_ONLY);
    while(bus->getOperatingMode() != CAN::LISTEN_ONLY);

    Serial.println("Done.");
}

/*
 * Check to see if a packet has been received. If so, read the packet and print
 * the packet payload to the serial monitor.
 */
// TODO does this need to be a different function so the volatile bool is
// pointing at the correct place? probably yes
void receiveCan(CAN* bus, volatile bool* messageReceived) {
    CAN::RxMessageBuffer* message;

    if(*messageReceived == false) {
        // The flag is updated by the CAN ISR.
        return;
    }

    message = bus->getRxMessage(CAN::CHANNEL1);
    decodeCanMessage(message->msgSID.SID, message->data);

    /* Call the CAN::updateChannel() function to let the CAN module know that
     * the message processing is done. Enable the event so that the CAN module
     * generates an interrupt when the event occurs.*/
    bus->updateChannel(CAN::CHANNEL1);
    bus->enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);

    *messageReceived = false;
}

/* Called by the Interrupt Service Routine whenever an event we registered for
 * occurs - this is where we wake up and decide to process a message.
 */
void handleCan1Interrupt() {
    if((can1.getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(can1.getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            can1.enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);
            isCan1MessageReceived = true;
        }
    }
}

void handleCan2Interrupt() {
    if((can2.getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(can2.getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            can2.enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);
            isCan2MessageReceived = true;
        }
    }
}
