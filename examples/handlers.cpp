#include "can/canread.h"

using openxc::can::read::publishNumericalMessage;

void handleSteeringWheelMessage(const CanSignal* signal, const CanSignal* signals, 
	SignalManager* signalManager, SignalManager* signalManagers, int signalCount, 
	CanMessage* message, Pipeline* pipeline) {
    publishNumericalMessage("latitude", 42.0, pipeline);
}

openxc_DynamicField handleInverted(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    return openxc::payload::wrapNumber(value * -1);
}

void initializeMyStuff() { }

void initializeOtherStuff() { }

void myLooper() {
    // this function will be called once each time through the main loop, after
    // all CAN message processing has been completed
}
