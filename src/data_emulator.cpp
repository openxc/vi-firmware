#include "data_emulator.h"
#include "can/canread.h"
#include "util/log.h"
#include "util/timer.h"
#include "signals.h"
#include <stdlib.h>

#define MAX_EMULATED_MESSAGES 1000
#define NUMERICAL_SIGNAL_COUNT 10
#define BOOLEAN_SIGNAL_COUNT 5
#define STATE_SIGNAL_COUNT 2
#define EVENT_SIGNAL_COUNT 2
#define EMULATOR_SEND_FREQUENCY 500

using openxc::can::read::publishNumericalMessage;
using openxc::can::read::publishBooleanMessage;
using openxc::can::read::publishStringMessage;
using openxc::can::read::publishStringEventedMessage;
using openxc::can::read::publishStringEventedBooleanMessage;
using openxc::pipeline::Pipeline;

static const char* NUMERICAL_SIGNALS[NUMERICAL_SIGNAL_COUNT] = {
    "steering_wheel_angle",
    "torque_at_transmission",
    "engine_speed",
    "vehicle_speed",
    "accelerator_pedal_position",
    "odometer",
    "latitude",
    "longitude",
    "fuel_level",
    "fuel_consumed_since_restart",
};

static const char* BOOLEAN_SIGNALS[BOOLEAN_SIGNAL_COUNT] = {
    "parking_brake_status",
    "brake_pedal_status",
    "headlamp_status",
    "high_beam_status",
    "windshield_wiper_status",
};

static const char* STATE_SIGNALS[STATE_SIGNAL_COUNT] = {
    "transmission_gear_position",
    "ignition_status",
};

static const char* EMULATED_SIGNAL_STATES[STATE_SIGNAL_COUNT][3] = {
    { "neutral", "first", "second" },
    { "off", "run", "accessory" },
};

static const char* EVENT_SIGNALS[EVENT_SIGNAL_COUNT] = {
   "door_status",
   "button_event",
};

static const char* EVENT_SIGNAL_STATES[EVENT_SIGNAL_COUNT][4] = {
   { "driver", "passenger", "rear_left", "rear_right" },
   { "up", "left", "down", "right" },
};

static const char* EVENT_SIGNAL_EVENT[EVENT_SIGNAL_COUNT][2] = {
   { "true", "false"},
   { "pressed", "released"},
};

static int messageCount = 0;
static bool unlimitedEmulatedMessages = true;

void openxc::emulator::restart() {
    messageCount = 0;
}

void openxc::emulator::generateFakeMeasurements(Pipeline* pipeline) {
    static int emulatorRateLimiter = 0;
    if(unlimitedEmulatedMessages || messageCount < MAX_EMULATED_MESSAGES) {
        ++emulatorRateLimiter;
        ++messageCount;
        if(emulatorRateLimiter == EMULATOR_SEND_FREQUENCY / 2) {
            publishNumericalMessage(NUMERICAL_SIGNALS[rand() % NUMERICAL_SIGNAL_COUNT],
			rand() % 50 + rand() % 100 * .1, pipeline);
            publishBooleanMessage(BOOLEAN_SIGNALS[rand() % BOOLEAN_SIGNAL_COUNT],
			rand() % 2 == 1 ? true : false, pipeline);
        } else if(emulatorRateLimiter == EMULATOR_SEND_FREQUENCY) {
            emulatorRateLimiter = 0;
            int stateSignalIndex = rand() % STATE_SIGNAL_COUNT;
            publishStringMessage(STATE_SIGNALS[stateSignalIndex], 
			EMULATED_SIGNAL_STATES[stateSignalIndex][rand() % 3], pipeline);
			//These signals are not reoccurring, so they are published at a
			//slower rate then all of the other signals. The % 50 allows them to
			//publish at a much more realistic rate
			if(rand() % 50 == 0){
				//door_status - cant just do a random call of the string like 
				//button event b/c emulator needs a boolean
				publishStringEventedBooleanMessage(EVENT_SIGNALS[0], 
				EVENT_SIGNAL_STATES[0][rand() % 4], rand() % 2 == 1 ? true : false, pipeline);
			}
			else if(rand() % 50 == 1){
				//button_event
				publishStringEventedMessage(EVENT_SIGNALS[1], 
				EVENT_SIGNAL_STATES[1][rand() % 4], EVENT_SIGNAL_EVENT[1][rand() % 2], pipeline);
			}
        }
    }
}
