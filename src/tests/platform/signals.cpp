#include "can/canread.h"
#include "can/canwrite.h"
#include "signals.h"
#include "util/log.h"
#include "config.h"
#include "shared_handlers.h"
#include "diagnostics.h"

namespace can = openxc::can;

using openxc::pipeline::Pipeline;
using openxc::config::getConfiguration;
using openxc::diagnostics::DiagnosticsManager;
using namespace openxc::signals::handlers;
using openxc::can::lookupSignalManagerDetails;


const int MESSAGE_SET_COUNT = 2;
CanMessageSet MESSAGE_SETS[MESSAGE_SET_COUNT] = {
    { 0, "tests", 2, 4, 23, 1 },
    { 1, "shared_handler_tests", 2, 4, 13, 0 },
};

const int MAX_CAN_BUS_COUNT = 2;
CanBus CAN_BUSES[][MAX_CAN_BUS_COUNT] = {
    { // message set: passthrough
        {
            speed: 500000,
            address: 1,
            maxMessageFrequency: 0,
            rawWritable: false
        },

        {
            speed: 125000,
            address: 2,
            maxMessageFrequency: 1,
            rawWritable: false
        },

    },
};

const int MAX_MESSAGE_COUNT = 6;
CanMessageDefinition CAN_MESSAGES[][MAX_MESSAGE_COUNT] = {
    { // message set: passthrough
        {&CAN_BUSES[0][0], 0},
        {&CAN_BUSES[0][0], 1, CanMessageFormat::STANDARD, {10}},
        {&CAN_BUSES[0][0], 2, CanMessageFormat::STANDARD, {1}, true},
        {&CAN_BUSES[0][0], 3},
        {&CAN_BUSES[0][0], 4},
        {&CAN_BUSES[0][0], 5}
    },
    { // message set: shared_handler_tests
        {&CAN_BUSES[1][0], 0},
        {&CAN_BUSES[1][0], 1},
        {&CAN_BUSES[1][0], 2},
        {&CAN_BUSES[1][0], 3}
    },
};

const int MAX_SIGNAL_COUNT = 29;
const CanSignalState SIGNAL_STATES[][2][6] = {
    { // message set: passthrough
        { {1, "reverse"}, {2, "third"}, {3, "sixth"}, {4, "seventh"},
            {5, "neutral"}, {6, "second"} },
        {{0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}}
    },
    { // message set: shared_handler_tests
        { {1, "right"}, {2, "down"}, {3, "left"}, {4, "ok"}, {5, "up"}, {6, "foo"}},
        { {1, "idle"}, {2, "stuck"}, {3, "held_short"}, {4, "pressed"},
            {5, "held_long"}, {6, "released"} }
    },
};

CanSignal SIGNALS[][MAX_SIGNAL_COUNT] = {
    { // message set: tests
        {&CAN_MESSAGES[0][0], "torque_at_transmission", 2, 4, 1001.0, -30000.000000,
            -5000.000000, 33522.000000, 0, false, false, NULL},
        {&CAN_MESSAGES[0][1], "transmission_gear_position", 1, 3, 1.000000, 0.000000,
            0.000000, 0.000000, 0, false, false, SIGNAL_STATES[0][0], 6, true, NULL, NULL},
        {&CAN_MESSAGES[0][2], "brake_pedal_status", 0, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][3], "measurement", 2, 19, 0.001000, 0.000000, 0, 500.0,
            0, false, false, SIGNAL_STATES[0][0], 6, true, NULL, NULL},
        {&CAN_MESSAGES[0][2], "command", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000},
        {&CAN_MESSAGES[0][2], "command", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000, 0,
            false, false, NULL},
        {&CAN_MESSAGES[0][0], "torque_at_transmission", 2, 6, 1001.0, -30000.000000,
            -5000.000000, 33522.000000, 0, false, false, NULL},

        // The messages for test_translate_many_signals, with 8 signals per
        // message to replicate a bug reported in the discussion group
        {&CAN_MESSAGES[0][4], "test_signal1", 0, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][4], "test_signal2", 1, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][4], "test_signal3", 2, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][4], "test_signal4", 3, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][4], "test_signal5", 4, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][4], "test_signal6", 5, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][4], "test_signal7", 6, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][4], "test_signal8", 7, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][5], "test_signal9", 0, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][5], "test_signa10", 1, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][5], "test_signa11", 2, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL,},
        {&CAN_MESSAGES[0][5], "test_signa12", 3, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][5], "test_signa13", 4, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][5], "test_signa14", 5, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][5], "test_signa15", 6, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL},
        {&CAN_MESSAGES[0][5], "test_signa16", 7, 1, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, NULL}
    },
    { // message set: shared_handler_tests
        {&CAN_MESSAGES[1][0], "button_type", 8, 8, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, SIGNAL_STATES[1][0], 5, false, NULL, NULL},
        {&CAN_MESSAGES[1][0], "button_state", 20, 4, 1.000000, 0.000000, 0.000000,
            0.000000, 0, true, false, SIGNAL_STATES[1][1], 6, false, NULL, NULL},
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
                0.000000, 0.000000}
    },
};

SignalManager SIGNAL_MANAGERS[][MAX_SIGNAL_COUNT] = {
    { // message set: type-3
        {signal: &SIGNALS[0][0]},
        {signal: &SIGNALS[0][1], received: false, lastValue: 4.0},
        {signal: &SIGNALS[0][2]},
        {signal: &SIGNALS[0][3], received: false, lastValue: 4.0},
        {signal: &SIGNALS[0][4]},
        {signal: &SIGNALS[0][5]},
        {signal: &SIGNALS[0][6]},
        {signal: &SIGNALS[0][7]},
        {signal: &SIGNALS[0][8]},
        {signal: &SIGNALS[0][9]},
        {signal: &SIGNALS[0][10]},
        {signal: &SIGNALS[0][11]},
        {signal: &SIGNALS[0][12]},
        {signal: &SIGNALS[0][13]},
        {signal: &SIGNALS[0][14]},
        {signal: &SIGNALS[0][15]},
        {signal: &SIGNALS[0][16]},
        {signal: &SIGNALS[0][17]},
        {signal: &SIGNALS[0][18]},
        {signal: &SIGNALS[0][19]},
        {signal: &SIGNALS[0][20]},
        {signal: &SIGNALS[0][21]},
        {signal: &SIGNALS[0][22]}
    },
    { // message set: type-3
        {signal: &SIGNALS[1][0]},
        {signal: &SIGNALS[1][1]},
        {signal: &SIGNALS[1][2]},
        {signal: &SIGNALS[1][3]},
        {signal: &SIGNALS[1][4]},
        {signal: &SIGNALS[1][5]},
        {signal: &SIGNALS[1][6]},
        {signal: &SIGNALS[1][7]},
        {signal: &SIGNALS[1][8]},
        {signal: &SIGNALS[1][9]},
        {signal: &SIGNALS[1][10]},
        {signal: &SIGNALS[1][11]},
        {signal: &SIGNALS[1][12]}
    },
};

void openxc::signals::initialize(DiagnosticsManager* diagnosticsManager) {
    switch(getConfiguration()->messageSetIndex) {
    case 0: // message set: tests
        break;
    }
}

void openxc::signals::loop() {
    switch(getConfiguration()->messageSetIndex) {
    case 0: // message set: tests
        break;
    }
}

char LAST_COMMAND_NAME[128];
openxc_DynamicField LAST_COMMAND_VALUE;
openxc_DynamicField LAST_COMMAND_EVENT;

void turnSignalCommandHandler(const char* name, openxc_DynamicField* value,
        openxc_DynamicField* event, const CanSignal* signals, int signalCount) {
    strcpy(LAST_COMMAND_NAME, name);
    LAST_COMMAND_VALUE = *value;
    if(event != NULL) {
        LAST_COMMAND_EVENT = *event;
    }
}

const int MAX_COMMAND_COUNT = 1;
CanCommand COMMANDS[][MAX_COMMAND_COUNT] = {
    { // message set: tests
        {"turn_signal_status", turnSignalCommandHandler},
    },
};

void openxc::signals::decodeCanMessage(Pipeline* pipeline, CanBus* bus, CanMessage* message) {
    switch(getConfiguration()->messageSetIndex) {
    case 0: // message set: tests
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

const CanMessageDefinition* openxc::signals::getMessages() {
    return CAN_MESSAGES[getActiveMessageSet()->index];
}

int openxc::signals::getMessageCount() {
    return getActiveMessageSet()->messageCount;
}

const CanSignal* openxc::signals::getSignals() {
    return (const CanSignal*) SIGNALS[getActiveMessageSet()->index];
}

SignalManager* openxc::signals::getSignalManagers() {
    return SIGNAL_MANAGERS[getActiveMessageSet()->index];
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

const CanMessageSet* openxc::signals::getActiveMessageSet() {
    return &MESSAGE_SETS[getConfiguration()->messageSetIndex];
}

const CanMessageSet* openxc::signals::getMessageSets() {
    return (const CanMessageSet*) MESSAGE_SETS;
}

int openxc::signals::getMessageSetCount() {
    return MESSAGE_SET_COUNT;
}
