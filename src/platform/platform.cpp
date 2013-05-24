#include "platform/platform.h"

#include "power.h"
#include "lights.h"
#include "bluetooth.h"
#include "signals.h"
#include "util/timer.h"
#include "util/log.h"
#include "can/canread.h"

using openxc::bluetooth::deinitializeBluetooth;
using openxc::can::deinitializeCan;
using openxc::power::enterLowPowerMode;
using openxc::lights::deinitializeLights;
using openxc::util::time::delayMs;
using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;

void openxc::platform::suspend(Pipeline* pipeline) {
    debug("CAN went silent - disabling LED");

    // De-init and shut down all peripherals to save power
    for(int i = 0; i < getCanBusCount(); ++i) {
        deinitializeCan(&getCanBuses()[i]);
    }
    deinitializeLights();
    deinitializeUsb(pipeline->usb);
    deinitializeBluetooth();

    // Wait for peripherals to disabled before sleeping
    delayMs(100);
    enterLowPowerMode();
}
