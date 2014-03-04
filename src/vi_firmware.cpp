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
#include "commands.h"

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
namespace commands = openxc::commands;

using openxc::util::log::debug;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalCount;
using openxc::signals::decodeCanMessage;
using openxc::pipeline::Pipeline;
using openxc::config::getConfiguration;

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

    usb::read(&getConfiguration()->usb, commands::handleIncomingMessage);
    uart::read(&getConfiguration()->uart, commands::handleIncomingMessage);
    network::read(&getConfiguration()->network, commands::handleIncomingMessage);

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
