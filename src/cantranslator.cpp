#ifndef CAN_EMULATOR

#include "usbutil.h"
#include "canread.h"
#include "serialutil.h"
#include "signals.h"
#include "log.h"
#include "cJSON.h"
#include "listener.h"
#include <stdint.h>

extern SerialDevice serialDevice;
extern UsbDevice USB_DEVICE;
extern Listener listener;

#ifdef __CHIPKIT__
CAN can1(CAN::CAN1);
CAN can2(CAN::CAN2);

// This is a reference to the last packet read
extern volatile CTRL_TRF_SETUP SetupPkt;
#endif // __CHIPKIT__

/* Forward declarations */

#ifdef __CHIPKIT__
static boolean usbCallback(USB_EVENT event, void *pdata, word size);
#endif // __CHIPKIT__

void initializeAllCan();
void receiveCan(CanBus*);
void checkIfStalled();
bool receiveWriteRequest(uint8_t*);

int receivedMessages = 0;
unsigned long lastSignificantChangeTime;
int receivedMessagesAtLastMark = 0;

void setup() {
    initializeLogging();
    initializeSerial(&serialDevice);
    initializeUsb(&USB_DEVICE);
    initializeAllCan();
#ifdef __CHIPKIT__
    lastSignificantChangeTime = millis();
#endif
}

void loop() {
    for(int i = 0; i < getCanBusCount(); i++) {
        receiveCan(&getCanBuses()[i]);
    }
    processListenerQueues(&listener);
    readFromHost(&USB_DEVICE, &receiveWriteRequest);
    readFromSerial(&serialDevice, &receiveWriteRequest);
    for(int i = 0; i < getCanBusCount(); i++) {
        processCanWriteQueue(&getCanBuses()[i]);
    }
    checkIfStalled();
}

#ifdef __CHIPKIT__

void initializeAllCan() {
    for(int i = 0; i < getCanBusCount(); i++) {
        initializeCan(&(getCanBuses()[i]));
    }
}

void mark() {
    lastSignificantChangeTime = millis();
    receivedMessagesAtLastMark = receivedMessages;
}

void checkIfStalled() {
    // a workaround to stop CAN from crashing indefinitely
    // See these tickets in Redmine:
    // https://fiesta.eecs.umich.edu/issues/298
    // https://fiesta.eecs.umich.edu/issues/244
    if(receivedMessagesAtLastMark + 10 < receivedMessages) {
        mark();
    }

    if(receivedMessages > 0 && receivedMessagesAtLastMark > 0
            && millis() > lastSignificantChangeTime + 500) {
        initializeAllCan();
        delay(1000);
        mark();
    }
}

bool receiveWriteRequest(uint8_t* message) {
    cJSON *root = cJSON_Parse((char*)message);
    if(root != NULL) {
        cJSON* nameObject = cJSON_GetObjectItem(root, "name");
        if(nameObject == NULL) {
            debug("Write request is malformed, missing name");
            return true;
        }
        char* name = nameObject->valuestring;
        CanSignal* signal = lookupSignal(name, getSignals(),
                getSignalCount(), true);
        if(signal != NULL) {
            cJSON* value = cJSON_GetObjectItem(root, "value");
            CanCommand* command = lookupCommand(name, getCommands(),
                    getCommandCount());
            if(command != NULL) {
                command->handler(name, value, getSignals(),
                        getSignalCount());
            } else {
                sendCanSignal(signal, value, getSignals(),
                        getSignalCount());
            }
        } else {
            debug("Writing not allowed for signal with name %s", name);
        }
        cJSON_Delete(root);
        return true;
    }
    return false;
}

/*
 * Check to see if a packet has been received. If so, read the packet and print
 * the packet payload to the serial monitor.
 */
void receiveCan(CanBus* bus) {
    CAN::RxMessageBuffer* message;

    if(bus->messageReceived == false) {
        // The flag is updated by the CAN ISR.
        return;
    }
    ++receivedMessages;

    message = bus->bus->getRxMessage(CAN::CHANNEL1);
    decodeCanMessage(message->msgSID.SID, message->data);

    /* Call the CAN::updateChannel() function to let the CAN module know that
     * the message processing is done. Enable the event so that the CAN module
     * generates an interrupt when the event occurs.*/
    bus->bus->updateChannel(CAN::CHANNEL1);
    bus->bus->enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);

    bus->messageReceived = false;
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

static boolean customUSBCallback(USB_EVENT event, void* pdata, word size) {
    switch(SetupPkt.bRequest) {
    case VERSION_CONTROL_COMMAND:
        char combinedVersion[strlen(VERSION) + strlen(getMessageSet()) + 2];
        sprintf(combinedVersion, "%s (%s)", VERSION, getMessageSet());
        debug("Version: %s (%s)", combinedVersion);

        USB_DEVICE.device.EP0SendRAMPtr((uint8_t*)combinedVersion,
                strlen(combinedVersion), USB_EP0_INCLUDE_ZERO);
        return true;
    case RESET_CONTROL_COMMAND:
        debug("Resetting...");
        initializeAllCan();
        return true;
    default:
        return false;
    }
}

static boolean usbCallback(USB_EVENT event, void *pdata, word size) {
    // initial connection up to configure will be handled by the default
    // callback routine.
    USB_DEVICE.device.DefaultCBEventHandler(event, pdata, size);

    switch(event) {
    case EVENT_CONFIGURED:
        debug("USB Configured");
        USB_DEVICE.configured = true;
        mark();
        USB_DEVICE.device.EnableEndpoint(DATA_ENDPOINT,
                USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|
                USB_DISALLOW_SETUP);
        break;

    case EVENT_EP0_REQUEST:
        customUSBCallback(event, pdata, size);
        break;

    default:
        break;
    }
}

#endif // CHIPKIT

#endif // CAN_EMULATOR
