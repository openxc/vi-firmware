/*
 *  Emulates being plugged into a live CAN network by output randomly valued
 *  JSON messages over USB.
 */

#include "WProgram.h"
#include "chipKITUSBDevice.h"
#include "usbutil.h"
#include "canutil.h"

#define NUMERICAL_SIGNAL_COUNT 11
#define BOOLEAN_SIGNAL_COUNT 5
#define STATE_SIGNAL_COUNT 2
#define EVENT_SIGNAL_COUNT 1

#define BOOLEAN_EVENT_MESSAGE_FORMAT "{\"name\":\"%s\",\"value\":\"%s\",\"event\":%s}\r\n"
#define BOOLEAN_EVENT_MESSAGE_VALUE_MAX_LENGTH 11

const int BOOLEAN_EVENT_MESSAGE_FORMAT_LENGTH = strlen(
        BOOLEAN_EVENT_MESSAGE_FORMAT);

// USB
#define DATA_ENDPOINT 1
CanUsbDevice usbDevice = {USBDevice(usbCallback), DATA_ENDPOINT, ENDPOINT_SIZE};

char* NUMERICAL_SIGNALS[NUMERICAL_SIGNAL_COUNT] = {
    "steering_wheel_angle",
    "powertrain_torque",
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

char* BOOLEAN_SIGNALS[BOOLEAN_SIGNAL_COUNT] = {
    "parking_brake_status",
    "brake_pedal_status",
    "headlamp_status",
    "high_beam_status",
    "windshield_wiper_status",
};

char* STATE_SIGNALS[STATE_SIGNAL_COUNT] = {
    "transmission_gear_position",
    "ignition_status",
};

char* SIGNAL_STATES[STATE_SIGNAL_COUNT][3] = {
    { "neutral", "first", "second" },
    { "off", "run", "accessory" },
};

char* EVENT_SIGNALS[EVENT_SIGNAL_COUNT] = {
    "door_status",
};

struct Event {
    char* value;
    bool event;
};

Event EVENT_SIGNAL_STATES[EVENT_SIGNAL_COUNT][3] = {
    { {"driver", false}, {"passenger", true}, {"rear_right", true}},
};

void writeNumericalMeasurement(char* measurementName, float value) {
    int messageLength = NUMERICAL_MESSAGE_FORMAT_LENGTH +
        strlen(measurementName) + NUMERICAL_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, NUMERICAL_MESSAGE_FORMAT, measurementName, value);

    sendMessage(&usbDevice, (uint8_t*) message, strlen(message));
}

void writeBooleanMeasurement(char* measurementName, bool value) {
    int messageLength = BOOLEAN_MESSAGE_FORMAT_LENGTH +
        strlen(measurementName) + BOOLEAN_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, BOOLEAN_MESSAGE_FORMAT, measurementName,
            value ? "true" : "false");

    sendMessage(&usbDevice, (uint8_t*) message, strlen(message));
}

void writeStateMeasurement(char* measurementName, char* value) {
    int messageLength = STRING_MESSAGE_FORMAT_LENGTH +
        strlen(measurementName) + STRING_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, STRING_MESSAGE_FORMAT, measurementName, value);

    sendMessage(&usbDevice, (uint8_t*) message, strlen(message));
}

void writeEventMeasurement(char* measurementName, Event event) {
    int messageLength = BOOLEAN_EVENT_MESSAGE_FORMAT_LENGTH +
        strlen(measurementName) + BOOLEAN_EVENT_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, BOOLEAN_EVENT_MESSAGE_FORMAT, measurementName, event.value,
            event.event ? "true" : "false");

    sendMessage(&usbDevice, (uint8_t*) message, strlen(message));
}

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));

    initializeUsb(&usbDevice);
}

void loop() {
    while(1) {
        writeNumericalMeasurement(
                NUMERICAL_SIGNALS[random(NUMERICAL_SIGNAL_COUNT)],
                random(101) + random(100) * .1);
        writeBooleanMeasurement(BOOLEAN_SIGNALS[random(BOOLEAN_SIGNAL_COUNT)],
                random(2) == 1 ? true : false);

        int stateSignalIndex = random(STATE_SIGNAL_COUNT);
        writeStateMeasurement(STATE_SIGNALS[stateSignalIndex],
                SIGNAL_STATES[stateSignalIndex][random(3)]);

        int eventSignalIndex = random(EVENT_SIGNAL_COUNT);
        writeEventMeasurement(EVENT_SIGNALS[eventSignalIndex],
                EVENT_SIGNAL_STATES[eventSignalIndex][random(3)]);
    }
}

static boolean usbCallback(USB_EVENT event, void *pdata, word size) {
    usbDevice.device.DefaultCBEventHandler(event, pdata, size);

    switch(event) {
    case EVENT_CONFIGURED:
        usbDevice.device.EnableEndpoint(DATA_ENDPOINT,
                USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        break;

    default:
        break;
    }
}
