#ifdef EMULATE_VEHICLE_DATA

#include "can/canread.h"
#include "can/canwrite.h"
#include "signals.h"
#include "util/log.h"
#include "config.h"
#include "shared_handlers.h"
#include "diagnostics.h"

namespace can = openxc::can;

using openxc::pipeline::Pipeline;
using openxc::diagnostics::DiagnosticsManager;

const int MESSAGE_SET_COUNT = 1;
const CanMessageSet MESSAGE_SETS[MESSAGE_SET_COUNT] = {
    { 0, "emulator", 0, 0, 0, 0 },
};

const int MAX_CAN_BUS_COUNT = 2;
CanBus CAN_BUSES[][MAX_CAN_BUS_COUNT] = {
    { // message set: emulator
    },
};

const int MAX_MESSAGE_COUNT = 0;
const CanMessageDefinition CAN_MESSAGES[][MAX_MESSAGE_COUNT] = {
};

const int MAX_SIGNAL_STATES = 0;
const int MAX_SIGNAL_COUNT = 0;
const CanSignalState SIGNAL_STATES[][MAX_SIGNAL_COUNT][MAX_SIGNAL_STATES] = {
};

CanSignal SIGNALS[][MAX_SIGNAL_COUNT] = {
};

void openxc::signals::initialize(DiagnosticsManager* diagnosticsManager) {
}

void openxc::signals::loop() {
}

const int MAX_COMMAND_COUNT = 1;
CanCommand COMMANDS[][MAX_COMMAND_COUNT] = {
};

void openxc::signals::decodeCanMessage(Pipeline* pipeline, const CanBus* bus, CanMessage* message) {
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

const CanMessageSet* openxc::signals::getActiveMessageSet() {
    return &MESSAGE_SETS[0];
}

const CanMessageSet* openxc::signals::getMessageSets() {
    return MESSAGE_SETS;
}

int openxc::signals::getMessageSetCount() {
    return MESSAGE_SET_COUNT;
}

#endif // EMULATE_VEHICLE_DATA
