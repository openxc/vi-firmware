/*
 *  Emulates being plugged into a live CAN network by output randomly valued
 *  JSON messages over USB.
 */

#include "chipKITUSBDevice.h"
#include "usbutil.h"
#include "canread.h"
#include "canutil.h"
#include "serialutil.h"
#include "listener.h"

#define NUMERICAL_SIGNAL_COUNT 11
#define BOOLEAN_SIGNAL_COUNT 5
#define STATE_SIGNAL_COUNT 2
#define EVENT_SIGNAL_COUNT 1

static boolean usbCallback(USB_EVENT event, void *pdata, word size);

// USB
#define DATA_ENDPOINT 1
SerialDevice serialDevice = {&Serial1};
UsbDevice usbDevice = {USBDevice(usbCallback), DATA_ENDPOINT,
        ENDPOINT_SIZE, &serialDevice};
Listener listener = {&usbDevice, &serialDevice};

char* NUMERICAL_SIGNALS[NUMERICAL_SIGNAL_COUNT] = {
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

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));

    initializeSerial(&serialDevice);
    initializeUsb(&usbDevice);
}

void loop() {
    while(1) {
        sendNumericalMessage(
                NUMERICAL_SIGNALS[random(NUMERICAL_SIGNAL_COUNT)],
                random(50) + random(100) * .1, &listener);
        sendBooleanMessage(BOOLEAN_SIGNALS[random(BOOLEAN_SIGNAL_COUNT)],
                random(2) == 1 ? true : false, &listener);

        int stateSignalIndex = random(STATE_SIGNAL_COUNT);
        sendStringMessage(STATE_SIGNALS[stateSignalIndex],
                SIGNAL_STATES[stateSignalIndex][random(3)], &listener);

        int eventSignalIndex = random(EVENT_SIGNAL_COUNT);
        Event randomEvent = EVENT_SIGNAL_STATES[eventSignalIndex][random(3)];
        sendEventedBooleanMessage(EVENT_SIGNALS[eventSignalIndex],
                randomEvent.value, randomEvent.event, &listener);
        processListenerQueues(&listener);
    }
}

int main(void) {
	init();
	setup();

	for (;;)
		loop();

	return 0;
}

static boolean usbCallback(USB_EVENT event, void *pdata, word size) {
    usbDevice.device.DefaultCBEventHandler(event, pdata, size);

    switch(event) {
    case EVENT_CONFIGURED:
        usbDevice.device.EnableEndpoint(DATA_ENDPOINT,
                USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        usbDevice.configured = true;
        break;

    default:
        break;
    }
}
