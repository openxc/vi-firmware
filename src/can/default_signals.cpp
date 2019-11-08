#include "diagnostics.h"
#include "can/canread.h"
#include "can/canwrite.h"
#include "signals.h"
#include "config.h"
#include "shared_handlers.h"
#include "util/log.h"

using openxc::util::log::debug;
using openxc::pipeline::Pipeline;
using openxc::diagnostics::DiagnosticsManager;

CanBus defaultBus = {
    speed: 500000,
    address: 1,
    maxMessageFrequency: 0,
    rawWritable: true
};

void openxc::signals::initialize(DiagnosticsManager* diagnosticsManager) { }

void openxc::signals::loop() { }

void openxc::signals::decodeCanMessage(Pipeline* pipeline, CanBus* bus, CanMessage* message) {
}

CanCommand* openxc::signals::getCommands() {
    return NULL;
}

int openxc::signals::getCommandCount() {
    return 0;
}

const CanMessageDefinition* openxc::signals::getMessages() {
    return NULL;
}

int openxc::signals::getMessageCount() {
    return 0;
}

const CanSignal* openxc::signals::getSignals() {
    return NULL;
}

int openxc::signals::getSignalCount() {
    return 0;
}

CanBus* openxc::signals::getCanBuses() {
    return &defaultBus;
}

int openxc::signals::getCanBusCount() {
    return 1;
}

const CanMessageSet* openxc::signals::getActiveMessageSet() {
    return NULL;
}

const CanMessageSet* openxc::signals::getMessageSets() {
    return NULL;
}

int openxc::signals::getMessageSetCount() {
    return 0;
}
