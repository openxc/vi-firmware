#ifdef CAN_EMULATOR

#include "interface/usb.h"
#include "can/canread.h"
#include "interface/uart.h"
#include "interface/network.h"
#include "util/log.h"
#include "util/timer.h"
#include "signals.h"
#include <stdlib.h>

#define NUMERICAL_SIGNAL_COUNT 11
#define BOOLEAN_SIGNAL_COUNT 5
#define STATE_SIGNAL_COUNT 2
#define EVENT_SIGNAL_COUNT 1
#define EMULATOR_SEND_FREQUENCY 500

namespace uart = openxc::interface::uart;
namespace network = openxc::interface::network;
namespace usb = openxc::interface::usb;

using openxc::can::read::sendNumericalMessage;
using openxc::can::read::sendBooleanMessage;
using openxc::can::read::sendStringMessage;
using openxc::can::read::sendEventedBooleanMessage;

int emulatorRateLimiter = 0;

extern Pipeline pipeline;

const char* NUMERICAL_SIGNALS[NUMERICAL_SIGNAL_COUNT] = {
    "steering_wheel_angle",
    "torque_at_transmission",
    "engine_speed",
    "vehicle_speed",
    "accelerator_pedal_position",
    "odometer",
    "fine_odometer_since_restart",
    "latitude",
    "longitude",
    "fuel_level",
    "fuel_consumed_since_restart",
};

const char* BOOLEAN_SIGNALS[BOOLEAN_SIGNAL_COUNT] = {
    "parking_brake_status",
    "brake_pedal_status",
    "headlamp_status",
    "high_beam_status",
    "windshield_wiper_status",
};

const char* STATE_SIGNALS[STATE_SIGNAL_COUNT] = {
    "transmission_gear_position",
    "ignition_status",
};

const char* EMULATED_SIGNAL_STATES[STATE_SIGNAL_COUNT][3] = {
    { "neutral", "first", "second" },
    { "off", "run", "accessory" },
};

const char* EVENT_SIGNALS[EVENT_SIGNAL_COUNT] = {
    "door_status",
};

struct Event {
    const char* value;
    bool event;
};

Event EVENT_SIGNAL_STATES[EVENT_SIGNAL_COUNT][3] = {
    { {"driver", false}, {"passenger", true}, {"rear_right", true}},
};

void setup() {
    srand(42);
}

bool usbWriteStub(uint8_t* buffer) {
    debug("Ignoring write request -- running an emulator");
    return true;
}

void loop() {
    ++emulatorRateLimiter;
    if(emulatorRateLimiter == EMULATOR_SEND_FREQUENCY / 2) {
        sendNumericalMessage(
                NUMERICAL_SIGNALS[rand() % NUMERICAL_SIGNAL_COUNT],
                rand() % 50 + rand() % 100 * .1, &pipeline);
        sendBooleanMessage(BOOLEAN_SIGNALS[rand() % BOOLEAN_SIGNAL_COUNT],
                rand() % 2 == 1 ? true : false, &pipeline);
    } else if(emulatorRateLimiter == EMULATOR_SEND_FREQUENCY) {
        emulatorRateLimiter = 0;

        int stateSignalIndex = rand() % STATE_SIGNAL_COUNT;
        sendStringMessage(STATE_SIGNALS[stateSignalIndex],
                EMULATED_SIGNAL_STATES[stateSignalIndex][rand() % 3], &pipeline);

        int eventSignalIndex = rand() % EVENT_SIGNAL_COUNT;
        Event randomEvent = EVENT_SIGNAL_STATES[eventSignalIndex][rand() % 3];
        sendEventedBooleanMessage(EVENT_SIGNALS[eventSignalIndex],
                randomEvent.value, randomEvent.event, &pipeline);
    }

    usb::read(pipeline.usb, usbWriteStub);
    uart::read(pipeline.uart, usbWriteStub);
}

void reset() { }

const int MESSAGE_SET_COUNT = 1;
CanMessageSet MESSAGE_SETS[MESSAGE_SET_COUNT] = {
    { 0, "emulator", 0, 0, 0 }
};

CanMessageSet* openxc::signals::getActiveMessageSet() {
    return &MESSAGE_SETS[0];
}

#endif // CAN_EMULATOR
