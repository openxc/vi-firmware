#ifdef CAN_EMULATOR

#include "usbutil.h"
#include "canread.h"
#include "serialutil.h"
#include "ethernetutil.h"
#include "log.h"
#include "timer.h"
#include <stdlib.h>

#define NUMERICAL_SIGNAL_COUNT 11
#define BOOLEAN_SIGNAL_COUNT 5
#define STATE_SIGNAL_COUNT 2
#define EVENT_SIGNAL_COUNT 1
#define EMULATOR_SEND_FREQUENCY 1000

int emulatorRateLimiter = 0;

extern Listener listener;

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

const char* SIGNAL_STATES[STATE_SIGNAL_COUNT][3] = {
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
                rand() % 50 + rand() % 100 * .1, &listener);
        sendBooleanMessage(BOOLEAN_SIGNALS[rand() % BOOLEAN_SIGNAL_COUNT],
                rand() % 2 == 1 ? true : false, &listener);
    } else if(emulatorRateLimiter == EMULATOR_SEND_FREQUENCY) {
        emulatorRateLimiter = 0;

        int stateSignalIndex = rand() % STATE_SIGNAL_COUNT;
        sendStringMessage(STATE_SIGNALS[stateSignalIndex],
                SIGNAL_STATES[stateSignalIndex][rand() % 3], &listener);

        int eventSignalIndex = rand() % EVENT_SIGNAL_COUNT;
        Event randomEvent = EVENT_SIGNAL_STATES[eventSignalIndex][rand() % 3];
        sendEventedBooleanMessage(EVENT_SIGNALS[eventSignalIndex],
                randomEvent.value, randomEvent.event, &listener);
    }

    readFromHost(listener.usb, usbWriteStub);
    readFromSerial(listener.serial, usbWriteStub);
}

void reset() { }

const char* getMessageSet() {
    return "emulator";
}

#endif // CAN_EMULATOR
