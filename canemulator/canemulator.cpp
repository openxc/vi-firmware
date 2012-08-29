/*
 *  Emulates being plugged into a live CAN network by output randomly valued
 *  JSON messages over USB.
 */

#include "usbutil.h"
#include "canread.h"
#include "canutil.h"
#include "serialutil.h"
#include "listener.h"
#include <stdlib.h>

#ifdef LPC1768
extern "C" {
#include "USB/USB.h"
}
#endif // LPC1768

#define NUMERICAL_SIGNAL_COUNT 11
#define BOOLEAN_SIGNAL_COUNT 5
#define STATE_SIGNAL_COUNT 2
#define EVENT_SIGNAL_COUNT 1

#ifdef CHIPKIT
static boolean usbCallback(USB_EVENT event, void *pdata, word size);
#endif

// USB
#define DATA_ENDPOINT 1

#ifdef CHIPKIT
SerialDevice serialDevice = {&Serial1};
#endif

#ifdef LPC1768
SerialDevice serialDevice;
#endif

UsbDevice USB_DEVICE = {
#ifdef CHIPKIT
    USBDevice(usbCallback),
#endif // CHIPKIT
    DATA_ENDPOINT,
    MAX_USB_PACKET_SIZE_BYTES};

Listener listener = {&USB_DEVICE, &serialDevice};

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
#ifdef CHIPKIT
    Serial.begin(115200);
#endif

    initializeSerial(&serialDevice);
    initializeUsb(&USB_DEVICE);
}

void loop() {
    while(1) {
        sendNumericalMessage(
                NUMERICAL_SIGNALS[rand() % NUMERICAL_SIGNAL_COUNT],
                rand() % 50 + rand() % 100 * .1, &listener);
        sendBooleanMessage(BOOLEAN_SIGNALS[rand() % BOOLEAN_SIGNAL_COUNT],
                rand() % 2 == 1 ? true : false, &listener);

        int stateSignalIndex = rand() % STATE_SIGNAL_COUNT;
        sendStringMessage(STATE_SIGNALS[stateSignalIndex],
                SIGNAL_STATES[stateSignalIndex][rand() % 3], &listener);

        int eventSignalIndex = rand() % EVENT_SIGNAL_COUNT;
        Event randomEvent = EVENT_SIGNAL_STATES[eventSignalIndex][rand() % 3];
        sendEventedBooleanMessage(EVENT_SIGNALS[eventSignalIndex],
                randomEvent.value, randomEvent.event, &listener);
        processListenerQueues(&listener);
    }
}

int main(void) {
#ifdef CHIPKIT
	init();
#endif
	setup();

	for (;;)
		loop();

	return 0;
}

#ifdef CHIPKIT
static boolean usbCallback(USB_EVENT event, void *pdata, word size) {
    USB_DEVICE.device.DefaultCBEventHandler(event, pdata, size);

    switch(event) {
    case EVENT_CONFIGURED:
        USB_DEVICE.device.EnableEndpoint(DATA_ENDPOINT,
                USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        USB_DEVICE.configured = true;
        break;

    default:
        break;
    }
}
#endif // CHIPKIT
