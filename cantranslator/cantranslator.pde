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
#define NODE_1_CAN_1_ADDRESS 0x101
#define NODE_1_CAN_2_ADDRESS 0x102

#define SYS_FREQ (80000000L)
#define HIGH_SPEED_CAN_BUS_SPEED 500000
#define INFOTAINMENT_CAN_BUS_SPEED 500000


/* High-speed CAN must be connected the 1st transceiver */
CAN highSpeedCan(CAN::CAN1);
/* Infotainment CAN must be connected the 2nd transceiver */
CAN infotainmentCan(CAN::CAN1);

/* CAN Message Buffers */
uint8_t highSpeedCanMessageArea[2 * 8 * 16];
uint8_t infotainmentCanMessageArea[2 * 8 * 16];

/* These are used as event flags by the interrupt service routines. */
static volatile bool isHighSpeedCanMessageReceived = false;
static volatile bool isInfotainmentCanMessageReceived = false;

/* Forward declarations */

void initializeCan(uint32_t);
void receiveCan(CAN*, volatile bool*);
void handleHighSpeedCanInterrupt();


void setup() {
    Serial.begin(115200);

    initializeUsb();

    initializeCan(&highSpeedCan, NODE_1_CAN_1_ADDRESS, HIGH_SPEED_CAN_BUS_SPEED,
            highSpeedCanMessageArea);
    initializeCan(&infotainmentCan, NODE_1_CAN_2_ADDRESS,
            INFOTAINMENT_CAN_BUS_SPEED, infotainmentCanMessageArea);

    highSpeedCan.attachInterrupt(handleHighSpeedCanInterrupt);
    infotainmentCan.attachInterrupt(handleInfotainmentCanInterrupt);
}

void loop() {
    receiveCan(&highSpeedCan, &isHighSpeedCanMessageReceived);
    receiveCan(&infotainmentCan, &isInfotainmentCanMessageReceived);
}

/* Initialize the CAN controller. See inline comments
 * for description of the process.
 */
void initializeCan(CAN* bus, uint32_t address, int speed, uint8_t* messageArea) {
    Serial.print("Initializing CAN bus at ");
    Serial.println(address, BYTE);
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

    // TODO need to initialize different filters and masks for each module
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
void handleHighSpeedCanInterrupt() {
    if((highSpeedCan.getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(highSpeedCan.getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            highSpeedCan.enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);
            isHighSpeedCanMessageReceived = true;
        }
    }
}

void handleInfotainmentCanInterrupt() {
    if((infotainmentCan.getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(infotainmentCan.getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            infotainmentCan.enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);
            isInfotainmentCanMessageReceived = true;
        }
    }
}
