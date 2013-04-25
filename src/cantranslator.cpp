#ifndef CAN_EMULATOR

#include "usbutil.h"
#include "canread.h"
#include "serialutil.h"
#include "networkutil.h"
#include "signals.h"
#include "log.h"
#include "cJSON.h"
#include "listener.h"
#include "timer.h"
#include "lights.h"
#include "power.h"
#include "bluetooth.h"
#include <stdint.h>
#include <stdlib.h>

using openxc::bluetooth::setBluetoothStatus;
using openxc::can::canBusActive;
using openxc::can::initializeCan;
using openxc::can::write::processCanWriteQueue;
using openxc::can::write::sendCanSignal;
using openxc::can::write::enqueueCanMessage;
using openxc::can::lookupCommand;
using openxc::can::lookupSignal;
using openxc::lights::LIGHT_A;
using openxc::lights::LIGHT_B;
using openxc::lights::COLORS;
using openxc::power::enterLowPowerMode;
using openxc::time::delayMs;
using openxc::time::systemTimeMs;
using openxc::signals::initializeSignals;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getCommands;
using openxc::signals::getCommandCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalCount;
using openxc::signals::decodeCanMessage;

extern Listener listener;

/* Forward declarations */

void receiveCan(Listener*, CanBus*);
void initializeAllCan();
bool receiveWriteRequest(uint8_t*);
void updateDataLights();

void setup() {
    initializeAllCan();
    initializeSignals();
}

void loop() {
    for(int i = 0; i < getCanBusCount(); i++) {
        receiveCan(&listener, &getCanBuses()[i]);
    }

    readFromHost(listener.usb, receiveWriteRequest);
    readFromSerial(listener.serial, receiveWriteRequest);
    readFromSocket(listener.network, receiveWriteRequest);

    for(int i = 0; i < getCanBusCount(); i++) {
        processCanWriteQueue(&getCanBuses()[i]);
    }

    updateDataLights();
}

/* Public: Update the color and status of a board's light that shows the status
 * of the CAN bus. This function is intended to be called each time through the
 * main program loop.
 */
void updateDataLights() {
    static bool busWasActive;
    bool busActive = false;
    for(int i = 0; i < getCanBusCount(); i++) {
        busActive = busActive || canBusActive(&getCanBuses()[i]);
    }

    if(!busWasActive && busActive) {
        debug("CAN woke up - enabling LED");
        enable(LIGHT_A, COLORS.blue);
        busWasActive = true;
    } else if(!busActive && busWasActive) {
#ifndef TRANSMITTER
        debug("CAN went silent - disabling LED");
        busWasActive = false;

        // TODO I don't love having all of this here, but it's the best place
        // for now. Maybe the modules need a deinitializeFoo() method in
        // addition to the initialize one.
        disable(LIGHT_A);
        disable(LIGHT_B);
        setBluetoothStatus(false);

        // Make sure lights and Bluetooth are disabled before sleeping
        delayMs(100);
        enterLowPowerMode();
#endif
    }
}

void initializeAllCan() {
    for(int i = 0; i < getCanBusCount(); i++) {
        initializeCan(&(getCanBuses()[i]));
    }
}

void receiveRawWriteRequest(cJSON* idObject, cJSON* root) {
    uint32_t id = idObject->valueint;
    cJSON* dataObject = cJSON_GetObjectItem(root, "data");
    if(dataObject == NULL) {
        debug("Raw write request missing data", id);
        return;
    }

    char* dataString = dataObject->valuestring;
    char* end;
    // TODO hard coding bus 0 right now, but it should support sending on either
    CanMessage message = {&getCanBuses()[0], id};
    enqueueCanMessage(&message, strtoull(dataString, &end, 16));
}

void receiveTranslatedWriteRequest(cJSON* nameObject, cJSON* root) {
    char* name = nameObject->valuestring;
    cJSON* value = cJSON_GetObjectItem(root, "value");

    // Optional, may be NULL
    cJSON* event = cJSON_GetObjectItem(root, "event");

    CanSignal* signal = lookupSignal(name, getSignals(), getSignalCount(),
            true);
    if(signal != NULL) {
        if(value == NULL) {
            debug("Write request for %s missing value", name);
            return;
        }
        sendCanSignal(signal, value, getSignals(), getSignalCount());
    } else {
        CanCommand* command = lookupCommand(name, getCommands(),
                getCommandCount());
        if(command != NULL) {
            command->handler(name, value, event, getSignals(),
                    getSignalCount());
        } else {
            debug("Writing not allowed for signal with name %s", name);
        }
    }
}

bool receiveWriteRequest(uint8_t* message) {
    cJSON *root = cJSON_Parse((char*)message);
    bool foundMessage = false;
    if(root != NULL) {
        foundMessage = true;
        cJSON* nameObject = cJSON_GetObjectItem(root, "name");
        if(nameObject == NULL) {
            cJSON* idObject = cJSON_GetObjectItem(root, "id");
            if(idObject == NULL) {
                debug("Write request is malformed, "
                        "missing name or id: %s", message);
            } else {
                receiveRawWriteRequest(idObject, root);
            }
        } else {
            receiveTranslatedWriteRequest(nameObject, root);
        }
        cJSON_Delete(root);
    } else {
        debug("No valid JSON in incoming buffer yet -- "
                "if it's valid, may be out of memory");
    }
    return foundMessage;
}

/*
 * Check to see if a packet has been received. If so, read the packet and print
 * the packet payload to the serial monitor.
 */
void receiveCan(Listener* listener, CanBus* bus) {
    // TODO what happens if we process until the queue is empty?
    if(!QUEUE_EMPTY(CanMessage, &bus->receiveQueue)) {
        CanMessage message = QUEUE_POP(CanMessage, &bus->receiveQueue);
        decodeCanMessage(listener, bus, message.id, message.data);
        bus->lastMessageReceived = systemTimeMs();
    }
}

void reset() {
    initializeAllCan();
}

#endif // CAN_EMULATOR
