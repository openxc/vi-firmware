#include "interface/usb.h"
#include "can/canread.h"
#include "interface/uart.h"
#include "interface/network.h"
#include "signals.h"
#include "util/log.h"
#include "cJSON.h"
#include "pipeline.h"
#include "util/timer.h"
#include "lights.h"
#include "power.h"
#include "bluetooth.h"
#include "platform/platform.h"
#include "diagnostics.h"
#include "data_emulator.h"
#include "config.h"

namespace uart = openxc::interface::uart;
namespace network = openxc::interface::network;
namespace usb = openxc::interface::usb;
namespace lights = openxc::lights;
namespace can = openxc::can;
namespace platform = openxc::platform;
namespace time = openxc::util::time;
namespace signals = openxc::signals;
namespace diagnostics = openxc::diagnostics;
namespace power = openxc::power;
namespace bluetooth = openxc::bluetooth;

using openxc::util::log::debug;
using openxc::can::lookupBus;
using openxc::can::lookupCommand;
using openxc::can::lookupSignal;
using openxc::signals::initialize;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getCommands;
using openxc::signals::getCommandCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalCount;
using openxc::signals::decodeCanMessage;
using openxc::pipeline::Pipeline;
using openxc::config::Configuration;
using openxc::config::getConfiguration;

/* Forward declarations */

void receiveCan(Pipeline*, CanBus*);
void initializeAllCan();
bool receiveWriteRequest(uint8_t*);
void updateDataLights();

static bool BUS_WAS_ACTIVE;

/* Public: Update the color and status of a board's light that shows the output
 * interface status. This function is intended to be called each time through
 * the main program loop.
 */
void updateInterfaceLight() {
    if(uart::connected(&getConfiguration()->uart)) {
        lights::enable(lights::LIGHT_B, lights::COLORS.blue);
    } else if(getConfiguration()->usb.configured) {
        lights::enable(lights::LIGHT_B, lights::COLORS.green);
    } else {
        lights::disable(lights::LIGHT_B);
    }
}

void initializeVehicleInterface() {
    platform::initialize();
    openxc::util::log::initialize();
    time::initialize();
    lights::initialize();
    power::initialize();
    usb::initialize(&getConfiguration()->usb);
    uart::initialize(&getConfiguration()->uart);

    updateInterfaceLight();

    bluetooth::initialize(&getConfiguration()->uart);
    network::initialize(&getConfiguration()->network);

    srand(time::systemTimeMs());

    debug("Initializing as %s", signals::getActiveMessageSet()->name);
    BUS_WAS_ACTIVE = false;
    initializeAllCan();
    diagnostics::initialize(&getConfiguration()->diagnosticsManager, getCanBuses(),
            getCanBusCount());
    signals::initialize(&getConfiguration()->diagnosticsManager);
}

void firmareLoop() {
    for(int i = 0; i < getCanBusCount(); i++) {
        // In normal operation, if no output interface is enabled/attached (e.g.
        // no USB or Bluetooth, the loop will stall here. Deep down in
        // receiveCan when it tries to append messages to the queue it will
        // reach a point where it tries to flush the (full) queue. Since nothing
        // is attached, that will just keep timing out. Just be aware that if
        // you need to modify the firmware to not use any interfaces, you'll
        // have to change that or enable the flush functionality to write to
        // your desired output interface.
        CanBus* bus = &(getCanBuses()[i]);
        receiveCan(&getConfiguration()->pipeline, bus);
        diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, bus);
    }


    usb::read(&getConfiguration()->usb, receiveWriteRequest);
    uart::read(&getConfiguration()->uart, receiveWriteRequest);
    network::read(&getConfiguration()->network, receiveWriteRequest);

    for(int i = 0; i < getCanBusCount(); i++) {
        can::write::processWriteQueue(&getCanBuses()[i]);
    }

    updateDataLights();
    openxc::signals::loop();
    can::logBusStatistics(getCanBuses(), getCanBusCount());
    openxc::pipeline::logStatistics(&getConfiguration()->pipeline);

#ifdef EMULATE_VEHICLE_DATA
    openxc::emulator::generateFakeMeasurements(&getConfiguration()->pipeline);
#endif // EMULATE_VEHICLE_DATA

    updateInterfaceLight();

    openxc::pipeline::process(&getConfiguration()->pipeline);
}

/* Public: Update the color and status of a board's light that shows the status
 * of the CAN bus. This function is intended to be called each time through the
 * main program loop.
 */
void updateDataLights() {
    bool busActive = false;
    for(int i = 0; i < getCanBusCount(); i++) {
        busActive = busActive || can::busActive(&getCanBuses()[i]);
    }

    if(!BUS_WAS_ACTIVE && busActive) {
        debug("CAN woke up - enabling LED");
        lights::enable(lights::LIGHT_A, lights::COLORS.blue);
        BUS_WAS_ACTIVE = true;
    } else if(!busActive && (BUS_WAS_ACTIVE || time::uptimeMs() >
            (unsigned long)openxc::can::CAN_ACTIVE_TIMEOUT_S * 1000)) {
        lights::enable(lights::LIGHT_A, lights::COLORS.red);
        BUS_WAS_ACTIVE = false;
#ifndef TRANSMITTER
#ifndef __DEBUG__
        // stay awake at least CAN_ACTIVE_TIMEOUT_S after power on
        platform::suspend(&getConfiguration()->pipeline);
#endif
#endif
    }
}

void initializeAllCan() {
    for(int i = 0; i < getCanBusCount(); i++) {
        CanBus* bus = &(getCanBuses()[i]);

        bool writable = bus->rawWritable ||
            can::signalsWritable(bus, getSignals(), getSignalCount());
#if defined(TRANSMITTER) || defined(__DEBUG__) || defined(__BENCHTEST__)
        // if we are bench testing with only 2 CAN nodes connected to one
        // another, the receiver must *not* be in listen only mode, otherwise
        // nobody ACKs the CAN messages and it looks like the whole things is
        // broken.
        writable = true;
#endif
        can::initialize(bus, writable, getCanBuses(), getCanBusCount());
    }
}

void receiveRawWriteRequest(cJSON* idObject, cJSON* root) {
    uint32_t id = idObject->valueint;
    cJSON* dataObject = cJSON_GetObjectItem(root, "data");
    if(dataObject == NULL) {
        debug("Raw write request missing data", id);
        return;
    }

    cJSON* busObject = cJSON_GetObjectItem(root, "bus");
    if(busObject == NULL) {
        debug("Raw write request missing bus", id);
    }

    CanBus* matchingBus = NULL;
    if(busObject != NULL) {
        int address = busObject->valueint;
        matchingBus = lookupBus(address, getCanBuses(), getCanBusCount());
        if(matchingBus == NULL) {
            debug("No matching active bus for requested address: %d", address);
        }
    }

    if(matchingBus == NULL) {
        debug("No matching bus for write request, so using the first we find");
        matchingBus = &getCanBuses()[0];
    }

    if(matchingBus->rawWritable) {
        char* dataString = dataObject->valuestring;
        char* end;
        CanMessage message = {id, strtoull(dataString, &end, 16)};
        can::write::enqueueMessage(matchingBus, &message);
    } else {
        debug("Raw CAN writes not allowed for bus %d", matchingBus->address);
    }
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
        can::write::sendSignal(signal, value, getSignals(), getSignalCount());
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
        // debug("No valid JSON in incoming buffer yet -- "
                // "if it's valid, may be out of memory");
    }
    return foundMessage;
}

/*
 * Check to see if a packet has been received. If so, read the packet and print
 * the packet payload to the uart monitor.
 */
void receiveCan(Pipeline* pipeline, CanBus* bus) {
    if(!QUEUE_EMPTY(CanMessage, &bus->receiveQueue)) {
        CanMessage message = QUEUE_POP(CanMessage, &bus->receiveQueue);
        decodeCanMessage(pipeline, bus, &message);
        bus->lastMessageReceived = time::systemTimeMs();

        ++bus->messagesReceived;

        diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, bus, &message, pipeline);
    }
}
