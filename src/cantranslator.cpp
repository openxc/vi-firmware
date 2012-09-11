#ifndef CAN_EMULATOR

#include "usbutil.h"
#include "canread.h"
#include "serialutil.h"
#include "signals.h"
#include "log.h"
#include "cJSON.h"
#include "listener.h"
#include <stdint.h>

extern SerialDevice SERIAL_DEVICE;
extern UsbDevice USB_DEVICE;
extern Listener listener;

/* Forward declarations */

void receiveCan(CanBus*);

#ifdef __PIC32__
void checkIfStalled();
#endif // __PIC32__

void initializeAllCan();
bool receiveWriteRequest(uint8_t*);

int receivedMessages = 0;
unsigned long lastSignificantChangeTime;
int receivedMessagesAtLastMark = 0;

void setup() {
    initializeLogging();
    initializeSerial(&SERIAL_DEVICE);
    initializeUsb(&USB_DEVICE);
    initializeAllCan();
#ifdef __PIC32__
    lastSignificantChangeTime = millis();
#endif
}

void loop() {
    for(int i = 0; i < getCanBusCount(); i++) {
        receiveCan(&getCanBuses()[i]);
    }
    processListenerQueues(&listener);
    readFromHost(&USB_DEVICE, &receiveWriteRequest);
    readFromSerial(&SERIAL_DEVICE, &receiveWriteRequest);
    for(int i = 0; i < getCanBusCount(); i++) {
        processCanWriteQueue(&getCanBuses()[i]);
    }
#ifdef __PIC32__
    checkIfStalled();
#endif // __PIC32__
}


void initializeAllCan() {
    for(int i = 0; i < getCanBusCount(); i++) {
        initializeCan(&(getCanBuses()[i]));
    }
}

bool receiveWriteRequest(uint8_t* message) {
    cJSON *root = cJSON_Parse((char*)message);
    if(root != NULL) {
        cJSON* nameObject = cJSON_GetObjectItem(root, "name");
        if(nameObject == NULL) {
            debug("Write request is malformed, missing name: %s\r\n", message);
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
                debug("Valid request: %s\r\n", message);
                sendCanSignal(signal, value, getSignals(),
                        getSignalCount());
            }
        } else {
            debug("Writing not allowed for signal with name %s\r\n", name);
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
    if(queue_length(&bus->receiveQueue) > 0) {
        CanMessage message = QUEUE_POP(CanMessage, &bus->receiveQueue);
        ++receivedMessages;
        decodeCanMessage(message.id, message.data);
    }
}

void reset() {
    initializeAllCan();
}

#ifdef __PIC32__

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

#endif // __PIC32__

#endif // CAN_EMULATOR
