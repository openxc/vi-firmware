#ifndef CAN_EMULATOR

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

namespace uart = openxc::interface::uart;
namespace network = openxc::interface::network;
namespace usb = openxc::interface::usb;
namespace lights = openxc::lights;
namespace can = openxc::can;
namespace platform = openxc::platform;
namespace time = openxc::util::time;
namespace signals = openxc::signals;
namespace diagnostics = openxc::diagnostics;

using openxc::util::log::debug;
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

extern Pipeline PIPELINE;
diagnostics::DiagnosticsManager DIAGNOSTICS_MANAGER;

/* Forward declarations */

void receiveCan(Pipeline*, CanBus*);
void initializeAllCan();
bool receiveWriteRequest(uint8_t*);
void updateDataLights();

void setup() {
    initializeAllCan();
    signals::initialize();
    diagnostics::initialize(&DIAGNOSTICS_MANAGER, getCanBuses(),
            getCanBusCount());

    // TODO this is temporary before we add the interface to add/remove messages
    // - just need one recurring request to test the system
    DiagnosticRequest request = {
        arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
        mode: OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST,
        pid: 0xc,
        pid_length: 1
    };
    diagnostics::addDiagnosticRequest(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &request, NULL, NULL, 1);
}

void loop() {
    for(int i = 0; i < getCanBusCount(); i++) {
        // In normal operation, if no output interface is enabled/attached (e.g.
        // no USB or Bluetooth, the loop will stall here. Deep down in
        // receiveCan when it tries to append messages to the queue it will
        // reach a point where it tries to flush the (full) queue. Since nothing
        // is attached, that will just keep timing out. Just be aware that if
        // you need to modify the firmware to not use any interfaces, you'll
        // have to change that or enable the flush functionality to write to
        // your desired output interface.
        receiveCan(&PIPELINE, &getCanBuses()[i]);
    }

    diagnostics::sendRequests(&DIAGNOSTICS_MANAGER);

    usb::read(PIPELINE.usb, receiveWriteRequest);
    uart::read(PIPELINE.uart, receiveWriteRequest);
    network::read(PIPELINE.network, receiveWriteRequest);

    for(int i = 0; i < getCanBusCount(); i++) {
        can::write::processWriteQueue(&getCanBuses()[i]);
    }

    updateDataLights();
    openxc::signals::loop();
    can::logBusStatistics(getCanBuses(), getCanBusCount());
    openxc::pipeline::logStatistics(&PIPELINE);
}

/* Public: Update the color and status of a board's light that shows the status
 * of the CAN bus. This function is intended to be called each time through the
 * main program loop.
 */
void updateDataLights() {
    static bool busWasActive;
    bool busActive = false;
    for(int i = 0; i < getCanBusCount(); i++) {
        busActive = busActive || can::busActive(&getCanBuses()[i]);
    }

    if(!busWasActive && busActive) {
        debug("CAN woke up - enabling LED");
        lights::enable(lights::LIGHT_A, lights::COLORS.blue);
        busWasActive = true;
    } else if(!busActive && (busWasActive || time::uptimeMs() >
            (unsigned long)openxc::can::CAN_ACTIVE_TIMEOUT_S * 1000)) {
        lights::enable(lights::LIGHT_A, lights::COLORS.red);
        busWasActive = false;
#ifndef TRANSMITTER
#ifndef __DEBUG__
        // stay awake at least CAN_ACTIVE_TIMEOUT_S after power on
        platform::suspend(&PIPELINE);
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
        can::initialize(getCanBuses(), getCanBusCount(), bus, writable);
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
        int busAddress = busObject->valueint;
        for(int i = 0; i < getCanBusCount(); i++) {
            CanBus* candidateBus = &(getCanBuses()[i]);
            if(candidateBus->address == busAddress) {
                matchingBus = candidateBus;
                break;
            }
        }

        if(matchingBus == NULL) {
            debug("No matching active bus for requested address: %d", busAddress);
        }
    }

    if(matchingBus == NULL) {
        debug("No matching bus for write request, so using the first we find");
        // TODO kill this...require bus
        matchingBus = &getCanBuses()[0];
    }

    char* dataString = dataObject->valuestring;
    char* end;
    CanMessage message = {id, strtoull(dataString, &end, 16)};
    can::write::enqueueMessage(matchingBus, &message);
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
        decodeCanMessage(pipeline, bus, message.id, message.data);
        bus->lastMessageReceived = time::systemTimeMs();

        ++bus->messagesReceived;

        diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, bus, &message, pipeline);
    }
}

#endif // CAN_EMULATOR
