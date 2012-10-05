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
void initializeAllCan();
bool receiveWriteRequest(uint8_t*);

void setup() {
    initializeLogging();
    initializeSerial(&SERIAL_DEVICE);
    initializeUsb(&USB_DEVICE);
    initializeAllCan();
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
}

void initializeAllCan() {
    for(int i = 0; i < getCanBusCount(); i++) {
        initializeCan(&(getCanBuses()[i]));
    }
}

bool receiveWriteRequest(uint8_t* message) {

    if((message[0] != '{') || (message[5] != '|') || (message[14] != '}'))
    {
      debug("Received a corrupted CAN message.\r\n");
      for(int i = 0; i < 16; i++) {
	debug("%02x ", message[i] );
      }
      debug("\r\n");
      return false;
    }

    CanMessage outGoing = {0, 0};
    for(int i = 1; i < 5; i++) {
	outGoing.id << 8;
	outGoing.id += message[i];
      }
    for(int i = 0; i < 8; i++) {
      ((uint8_t*)&(outGoing.data))[i] = message[i+6];
    }

    /*    debug("Sending CAN message id = 0x%02x, data = 0x", outGoing.id);
    for(int i = 0; i < 8; i++) {
      debug("%02x ", ((uint8_t*)&(outGoing.data))[i] );
    }
    debug("\r\n");
    */
    sendCanMessage(&(getCanBuses()[0]), outGoing);
      return true;

    cJSON *root = cJSON_Parse((char*)message);
    if(root != NULL) {
        cJSON* nameObject = cJSON_GetObjectItem(root, "name");
        if(nameObject == NULL) {
            debug("Write request is malformed, missing name: %s\r\n", message);
            return true;
        }
        char* name = nameObject->valuestring;
        cJSON* value = cJSON_GetObjectItem(root, "value");
        CanSignal* signal = lookupSignal(name, getSignals(),
                getSignalCount(), true);
        CanCommand* command = lookupCommand(name, getCommands(),
                getCommandCount());
        if(signal != NULL) {
            sendCanSignal(signal, value, getSignals(), getSignalCount());
        } else if(command != NULL) {
            command->handler(name, value, getSignals(), getSignalCount());
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
    // TODO what happens if we process until the queue is empty?
    if(!QUEUE_EMPTY(CanMessage, &bus->receiveQueue)) {
        CanMessage message = QUEUE_POP(CanMessage, &bus->receiveQueue);
	debug("%X\n\r", message.id);
        decodeCanMessage(message.id, message.data);
    }
}

void reset() {
    initializeAllCan();
}

#endif // CAN_EMULATOR
