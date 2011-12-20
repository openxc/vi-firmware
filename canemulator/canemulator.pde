/*
 *  Emulates being plugged into a live CAN network by output randomly valued
 *  JSON messages over USB.
 */

#include "WProgram.h"
#include "chipKITUSBDevice.h"
#include "usbutil.h"
#include "canutil.h"

#define NUMERICAL_SIGNAL_COUNT 10
#define BOOLEAN_SIGNAL_COUNT 4
#define STATE_SIGNAL_COUNT 2

char* NUMERICAL_SIGNALS[NUMERICAL_SIGNAL_COUNT] = {
    "steering_wheel_angle",
    "powertrain_torque",
    "engine_speed",
    "vehicle_speed",
    "accelerator_pedal_position",
    "odometer",
    "windshield_wiper_speed",
    "latitude",
    "longitude",
    "fuel_level",
};

char* BOOLEAN_SIGNALS[BOOLEAN_SIGNAL_COUNT] = {
    "parking_brake_status",
    "brake_pedal_status",
    "headlamp_status",
    "high_beam_status",
};

char* STATE_SIGNALS[STATE_SIGNAL_COUNT] = {
    "transmission_gear_position",
    "ignition_status",
};

CanSignalState SIGNAL_STATES[STATE_SIGNAL_COUNT][5] = {
    { {0, "N"}, {1, "1"}, {3, "3"}, {2, "2"}, {5, "5"}, },
};

void writeNumericalMeasurement(char* measurementName, float value) {
    int messageLength = NUMERICAL_MESSAGE_FORMAT_LENGTH +
        strlen(measurementName) + NUMERICAL_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, NUMERICAL_MESSAGE_FORMAT, measurementName, value);

    sendMessage((uint8_t*) message, strlen(message));
}

void writeBooleanMeasurement(char* measurementName, bool value) {
    int messageLength = BOOLEAN_MESSAGE_FORMAT_LENGTH +
        strlen(measurementName) + BOOLEAN_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, BOOLEAN_MESSAGE_FORMAT, measurementName,
            value ? "true" : "false");

    sendMessage((uint8_t*) message, strlen(message));
}

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));

    initializeUsb();
}

void loop() {
    while(1) {
        writeNumericalMeasurement(
                NUMERICAL_SIGNALS[random(NUMERICAL_SIGNAL_COUNT)],
                random(101) + random(100) * .1);
        writeBooleanMeasurement(BOOLEAN_SIGNALS[random(BOOLEAN_SIGNAL_COUNT)],
                random(2) == 1 ? true : false);
    }
}
