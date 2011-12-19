/*
 *  Emulates being plugged into a live CAN network by output randomly valued
 *  JSON messages over USB.
 */

#include "WProgram.h"
#include "chipKITUSBDevice.h"
#include "usbutil.h"

#define SIGNAL_COUNT 11

char* SIGNALS[SIGNAL_COUNT] = {
    "vehicle_speed",
    "transmission_gear_position",
    "accelerator_pedal_position",
    "windshield_wiper_speed",
    "steering_wheel_angle",
    "parking_brake_status",
    "brake_pedal_status",
    "engine_speed",
    "latitude",
    "longitude",
    "transmission_gear_pos",
};

void writeMeasurement(char* measurementName, float value) {
    int messageLength = NUMERICAL_MESSAGE_FORMAT_LENGTH +
        strlen(measurementName) + NUMERICAL_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, NUMERICAL_MESSAGE_FORMAT, measurementName, value);

    sendMessage((uint8_t*) message, strlen(message));
}

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));

    initializeUsb();
}

void loop() {
    while(1) {
        writeMeasurement(SIGNALS[random(SIGNAL_COUNT)],
                random(101) + random(100) * .1);
    }
}
