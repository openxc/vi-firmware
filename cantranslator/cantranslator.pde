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
#define node1can1 0x101L
#define node2can1 0x201L

#define SYS_FREQ (80000000L)
#define CAN_BUS_SPEED 500000


// this object uses CAN module 1
CAN canModule(CAN::CAN1);

/* CAN Message Buffers */
uint8_t canMessageFifoArea[2 * 8 * 16];

/* These are used as event flags by the interrupt service routines. */
static volatile bool isCanMessageReceived = false;

/* Forward declarations */

void initializeCan(uint32_t myaddr);
void receiveCan(void);
void handleCanInterrupt();
void decode_can_message(int id, uint8_t* data);


void setup() {
    Serial.begin(115200);

    initializeUsb();
    initializeCan(node1can1);
    canModule.attachInterrupt(handleCanInterrupt);
}

void loop() {
    receiveCan();
}

/* Initialize the CAN controller. See inline comments
 * for description of the process.
 */
void initializeCan(uint32_t myaddr) {
    Serial.print("Initializing CAN 1...  ");
    CAN::BIT_CONFIG canBitConfig;

    /* Switch the CAN module ON and switch it to Configuration mode. Wait till
     * the switch is complete */
    canModule.enableModule(true);
    canModule.setOperatingMode(CAN::CONFIGURATION);
    while(canModule.getOperatingMode() != CAN::CONFIGURATION);

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
    canModule.setSpeed(&canBitConfig, SYS_FREQ, CAN_BUS_SPEED);

    /* Assign the buffer area to the CAN module. */
    /* Note the size of each Channel area. It is 2 (Channels) * 8 (Messages
     * Buffers) 16 (bytes/per message buffer) bytes. Each CAN module should have
     * its own message area. */
    canModule.assignMemoryBuffer(canMessageFifoArea, 2 * 8 * 16);

    /* Configure channel 1 for RX and size of 8 message buffers and receive the
     * full message.
     */
    canModule.configureChannelForRx(CAN::CHANNEL1, 8, CAN::RX_FULL_RECEIVE);

    CanFilterMask* filterMasks = initializeFilterMasks();
    CanFilter* filters = initializeFilters();
    configureFilters(&canModule, filterMasks, filters);

    /* Enable interrupt and events. Enable the receive channel not empty
     * event (channel event) and the receive channel event (module event). The
     * interrrupt peripheral library is used to enable the CAN interrupt to the
     * CPU. */
    canModule.enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);
    canModule.enableModuleEvent(CAN::RX_EVENT, true);

    canModule.setOperatingMode(CAN::LISTEN_ONLY);
    while(canModule.getOperatingMode() != CAN::LISTEN_ONLY);

    Serial.println("Done.");
}

/*
 * Check to see if a packet has been received. If so, read the packet and print
 * the packet payload to the serial monitor.
 */
void receiveCan(void) {
    CAN::RxMessageBuffer* message;

    if(isCanMessageReceived == false) {
        // The isCanMessageReceived flag is updated by the CAN ISR.
        return;
    }

    message = canModule.getRxMessage(CAN::CHANNEL1);
    decodeCanMessage(message->msgSID.SID, message->data);

    /* Call the CAN::updateChannel() function to let the CAN module know that
     * the message processing is done. Enable the event so that the CAN module
     * generates an interrupt when the event occurs.*/
    canModule.updateChannel(CAN::CHANNEL1);
    canModule.enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);

    isCanMessageReceived = false;
}

/* Called by the Interrupt Service Routine whenever an event we registered for
 * occurs - this is where we wake up and decide to process a message.
 */
void handleCanInterrupt() {
    if((canModule.getModuleEvent() & CAN::RX_EVENT) != 0) {
        if(canModule.getPendingEventCode() == CAN::CHANNEL1_EVENT) {
            // Clear the event so we give up control of the CPU
            canModule.enableChannelEvent(CAN::CHANNEL1,
                    CAN::RX_CHANNEL_NOT_EMPTY, false);
            isCanMessageReceived = true;
        }
    }
}
