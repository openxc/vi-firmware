#include "can/canread.h"
#include "can/canwrite.h"
#include "signals.h"
#include "util/log.h"
#include "config.h"
#include "shared_handlers.h"

namespace can = openxc::can;

using openxc::util::log::debug;
using openxc::pipeline::Pipeline;
using openxc::config::getConfiguration;
using openxc::can::read::booleanHandler;
using openxc::can::read::stateHandler;
using openxc::can::read::ignoreHandler;
using openxc::can::write::booleanWriter;
using openxc::can::write::stateWriter;
using openxc::can::write::numberWriter;
using namespace openxc::signals::handlers;

const int MESSAGE_SET_COUNT = 2;
CanMessageSet MESSAGE_SETS[MESSAGE_SET_COUNT] = {
    { 0, "tests", 2, 4, 7, 1 },
    { 1, "shared_handler_tests", 2, 4, 13, 0 },
};

const int MAX_CAN_BUS_COUNT = 2;
CanBus CAN_BUSES[][MAX_CAN_BUS_COUNT] = {
    { // message set: passthrough
        { 500000, 1, NULL, 1, false
        },

        { 125000, 2, NULL, 1, false
        },

    },
};

const int MAX_MESSAGE_COUNT = 4;
CanMessageDefinition CAN_MESSAGES[][MAX_MESSAGE_COUNT] = {
    { // message set: passthrough
        {&CAN_BUSES[0][0], 0},
        {&CAN_BUSES[0][0], 1, {10}},
        {&CAN_BUSES[0][0], 2, {1}, true},
        {&CAN_BUSES[0][0], 3}
    },
    { // message set: shared_handler_tests
        {&CAN_BUSES[1][0], 0},
        {&CAN_BUSES[1][0], 1},
        {&CAN_BUSES[1][0], 2},
        {&CAN_BUSES[1][0], 3}
    },
};

const int MAX_SIGNAL_STATES = 12;
const int MAX_SIGNAL_COUNT = 13;
const CanSignalState SIGNAL_STATES[][MAX_SIGNAL_COUNT][MAX_SIGNAL_STATES] = {
    { // message set: passthrough
        { {1, "reverse"}, {2, "third"}, {3, "sixth"}, {4, "seventh"},
            {5, "neutral"}, {6, "second"}, },
    },
    { // message set: shared_handler_tests
        { {1, "right"}, {2, "down"}, {3, "left"}, {4, "ok"}, {5, "up"}, {6, "foo"}},
        { {1, "idle"}, {2, "stuck"}, {3, "held_short"}, {4, "pressed"},
            {5, "held_long"}, {6, "released"}, },
    },
};

CanSignal SIGNALS[][MAX_SIGNAL_COUNT] = {
    { // message set: passthrough
        {&CAN_MESSAGES[0][0], "torque_at_transmission", 2, 4, 1001.0, -30000.000000,
            -5000.000000, 33522.000000, {0}, false, false, NULL, 0, true},
        {&CAN_MESSAGES[0][1], "transmission_gear_position", 1, 3, 1.000000, 0.000000,
            0.000000, 0.000000, {0}, false, false, SIGNAL_STATES[0][0], 6, true, NULL,
            false, 4.0},
        {&CAN_MESSAGES[0][2], "brake_pedal_status", 0, 1, 1.000000, 0.000000, 0.000000,
            0.000000, {0}, true, false, NULL, 0, true},
        {&CAN_MESSAGES[0][3], "measurement", 2, 19, 0.001000, 0.000000, 0, 500.0,
            {0}, false, false, SIGNAL_STATES[0][0], 6, true, NULL, 4.0},
        {&CAN_MESSAGES[0][2], "command", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000},
        {&CAN_MESSAGES[0][2], "command", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000, {0},
            false, false, NULL, 0, true},
        {&CAN_MESSAGES[0][0], "torque_at_transmission", 2, 6, 1001.0, -30000.000000,
            -5000.000000, 33522.000000, {0}, false, false, NULL, 0, true},
    },
    { // message set: shared_handler_tests
        {&CAN_MESSAGES[1][0], "button_type", 8, 8, 1.000000, 0.000000, 0.000000,
            0.000000, {0}, true, false, SIGNAL_STATES[1][0], 5, false, NULL},
        {&CAN_MESSAGES[1][0], "button_state", 20, 4, 1.000000, 0.000000, 0.000000,
            0.000000, {0}, true, false, SIGNAL_STATES[1][1], 6, false, NULL},
        {&CAN_MESSAGES[1][1], "driver_door", 15, 1, 1.000000, 0.000000, 0.000000,
                0.000000},
        {&CAN_MESSAGES[1][1], "passenger_door", 16, 1, 1.000000, 0.000000, 0.000000,
                0.000000},
        {&CAN_MESSAGES[1][1], "rear_left_door", 17, 1, 1.000000, 0.000000, 0.000000,
                0.000000},
        {&CAN_MESSAGES[1][1], "rear_right_door", 18, 1, 1.000000, 0.000000, 0.000000,
                0.000000},
        {&CAN_MESSAGES[1][1], "fuel_consumed_since_restart", 18, 1, 25.000000, 0.000000,
                0.000000, 255.0},
        {&CAN_MESSAGES[1][2], "tire_pressure_front_left", 15, 1, 1.000000, 0.000000,
                0.000000, 0.000000},
        {&CAN_MESSAGES[1][2], "tire_pressure_front_right", 16, 1, 1.000000, 0.000000,
                0.000000, 0.000000},
        {&CAN_MESSAGES[1][2], "tire_pressure_rear_right", 17, 1, 1.000000, 0.000000,
                0.000000, 0.000000},
        {&CAN_MESSAGES[1][2], "tire_pressure_rear_left", 18, 1, 1.000000, 0.000000,
                0.000000, 0.000000},
        {&CAN_MESSAGES[1][3], "passenger_occupancy_lower", 17, 1, 1.000000, 0.000000,
                0.000000, 0.000000},
        {&CAN_MESSAGES[1][3], "passenger_occupancy_upper", 18, 1, 1.000000, 0.000000,
                0.000000, 0.000000},
    }
};

void openxc::signals::initialize() {
    switch(getConfiguration()->messageSetIndex) {
    case 0: // message set: passthrough
        break;
    }
}

void openxc::signals::loop() {
    switch(getConfiguration()->messageSetIndex) {
    case 0: // message set: passthrough
        break;
    }
}

const int MAX_COMMAND_COUNT = 1;
CanCommand COMMANDS[][MAX_COMMAND_COUNT] = {
    { // message set: passthrough
        {"turn_signal_status", NULL},
    },
};

void openxc::signals::decodeCanMessage(Pipeline* pipeline, CanBus* bus, CanMessage* message) {
    switch(getConfiguration()->messageSetIndex) {
    case 0: // message set: passthrough
        switch(bus->address) {
        case 1:
            switch (message->id) {
            }
            openxc::can::read::passthroughMessage(bus, message, getMessages(), getMessageCount(), pipeline);
            break;
        case 2:
            switch (message->id) {
            }
            openxc::can::read::passthroughMessage(bus, message, getMessages(), getMessageCount(), pipeline);
            break;
        }
        break;
    }
}


CanCommand* openxc::signals::getCommands() {
    return COMMANDS[getActiveMessageSet()->index];
}

int openxc::signals::getCommandCount() {
    return getActiveMessageSet()->commandCount;
}

CanMessageDefinition* openxc::signals::getMessages() {
    return CAN_MESSAGES[getActiveMessageSet()->index];
}

int openxc::signals::getMessageCount() {
    return getActiveMessageSet()->messageCount;
}

CanSignal* openxc::signals::getSignals() {
    return SIGNALS[getActiveMessageSet()->index];
}

int openxc::signals::getSignalCount() {
    return getActiveMessageSet()->signalCount;
}

CanBus* openxc::signals::getCanBuses() {
    return CAN_BUSES[getActiveMessageSet()->index];
}

int openxc::signals::getCanBusCount() {
    return getActiveMessageSet()->busCount;
}

CanMessageSet* openxc::signals::getActiveMessageSet() {
    return &MESSAGE_SETS[getConfiguration()->messageSetIndex];
}

CanMessageSet* openxc::signals::getMessageSets() {
    return MESSAGE_SETS;
}

int openxc::signals::getMessageSetCount() {
    return MESSAGE_SET_COUNT;
}
