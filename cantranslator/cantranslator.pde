/*
 *
 *  Derived from CanDemo.pde, example code that came with the
 *  chipKIT Network Shield libraries.
 */

#include "chipKITCAN.h"
#include "chipKITUSBDevice.h"
#include "bitfield.h"
#include "canutil_chipkit.h"
#include "usbutil.h"

#define CAN_BUS_1_SPEED 500000
#define CAN_BUS_2_SPEED 500000

#define VERSION_CONTROL_COMMAND 0x80
#define RESET_CONTROL_COMMAND 0x81
char* VERSION = "1.1-dev";
extern char* MESSAGE_SET;

CAN can1(CAN::CAN1);
CAN can2(CAN::CAN2);

USBDevice usbDevice(usbCallback);

/* CAN Message Buffers */
uint8_t can1MessageArea[2 * 8 * 16];
uint8_t can2MessageArea[2 * 8 * 16];

/* These are used as event flags by the interrupt service routines. */
static volatile bool isCan1MessageReceived = false;
static volatile bool isCan2MessageReceived = false;

/* Forward declarations */

void initializeAllCan();
void initializeCan(uint32_t);
void receiveCan(CAN*, volatile bool*);
void handleCan1Interrupt();
void handleCan2Interrupt();
void decodeCanMessage(int id, uint8_t* data);

void setup() {
    Serial.begin(115200);

    initializeUsb(&usbDevice);
    initializeAllCan();
}

void initializeAllCan() {
    initializeCan(&can1, CAN_1_ADDRESS, CAN_BUS_1_SPEED, can1MessageArea);
    initializeCan(&can2, CAN_2_ADDRESS, CAN_BUS_2_SPEED, can2MessageArea);

    can1.attachInterrupt(handleCan1Interrupt);
    can2.attachInterrupt(handleCan2Interrupt);
}

int receivedMessages = 0;

void loop() {
    receiveCan(&can1, &isCan1MessageReceived);
    receiveCan(&can2, &isCan2MessageReceived);

    // a workaround to stop CAN from crashing indefinitely
    // See these tickets in Redmine:
    // https://fiesta.eecs.umich.edu/issues/298
    // https://fiesta.eecs.umich.edu/issues/244
    if(receivedMessages % 1000 == 0) {
        initializeAllCan();
        delay(100);
    }
    ++receivedMessages;
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
 * occurs - this is where we wake up and decide to process a message. */
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


static boolean customUSBCallback(USB_EVENT event, void* pdata, word size) {
    switch(SetupPkt.bRequest) {
    case VERSION_CONTROL_COMMAND:
        char combined_version[100];

        Serial.print("Software version is ");
        Serial.print(VERSION);
        Serial.print("Message set ");
        Serial.println(MESSAGE_SET);

        strcpy(combined_version, VERSION);
        //Make sure we've got enough buffer
        if (strlen(MESSAGE_SET) + 1 + strlen(VERSION) < 100){
          combined_version[strlen(VERSION)] = ',';
          strcpy(combined_version+strlen(VERSION) + 1, MESSAGE_SET);
        }
        usbDevice.EP0SendRAMPtr(
            (uint8_t*)combined_version,
            strlen(combined_version),
            USB_EP0_INCLUDE_ZERO);
        return true;
    case RESET_CONTROL_COMMAND:
        Serial.print("Resetting...");
        initializeAllCan();
        return true;
    default:
        Serial.print("Didn't recognize event: ");
        Serial.println(SetupPkt.bRequest);
        return false;
    }
}

static boolean usbCallback(USB_EVENT event, void *pdata, word size) {
    // initial connection up to configure will be handled by the default
    // callback routine.
    usbDevice.DefaultCBEventHandler(event, pdata, size);

    switch(event) {
    case EVENT_CONFIGURED:
        Serial.println("Event: Configured");
        // Enable DATA_ENDPOINT for input
        usbDevice.EnableEndpoint(DATA_ENDPOINT,
                USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        break;

    case EVENT_EP0_REQUEST:
        if(!customUSBCallback(event, pdata, size)) {
            Serial.println("Event: Unrecognized EP0 (endpoint 0) Request");
        }
        break;

    default:
        break;
    }
}
