#include "diagnostics.h"
#include "can/canread.h"
#include "can/canwrite.h"
#include "signals.h"
#include "config.h"
#include "shared_handlers.h"

using openxc::pipeline::Pipeline;
using openxc::diagnostics::DiagnosticsManager;

#ifdef __LPC17XX__
#define can1 LPC_CAN1
#endif // __LPC17XX__

#ifdef __PIC32__
extern void* can1;
extern void handleCan1Interrupt();
#endif // __PIC32__

CanBus defaultBus = {
    125000, 1, can1,
        0, false,
        #ifdef __PIC32__
        handleCan1Interrupt,
        #endif // __PIC32__
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

CanMessageDefinition* openxc::signals::getMessages() {
    return NULL;
}

int openxc::signals::getMessageCount() {
    return 0;
}

CanSignal* openxc::signals::getSignals() {
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

CanMessageSet* openxc::signals::getActiveMessageSet() {
    return NULL;
}

CanMessageSet* openxc::signals::getMessageSets() {
    return NULL;
}

int openxc::signals::getMessageSetCount() {
    return 0;
}
