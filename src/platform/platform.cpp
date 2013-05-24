#include "platform/platform.h"

#include "power.h"
#include "lights.h"
#include "bluetooth.h"
#include "signals.h"
#include "util/timer.h"
#include "util/log.h"
#include "can/canread.h"

namespace usb = openxc::interface::usb;

using openxc::can::deinitialize;
using openxc::power::enterLowPowerMode;
using openxc::lights::deinitialize;
using openxc::util::time::delayMs;
using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;

void openxc::platform::suspend(Pipeline* pipeline) {
    debug("CAN went silent - disabling LED");

    // De-init and shut down all peripherals to save power
    for(int i = 0; i < getCanBusCount(); ++i) {
        can::deinitialize(&getCanBuses()[i]);
    }
    lights::deinitialize();
    usb::deinitialize(pipeline->usb);
    bluetooth::deinitialize();

    // Wait for peripherals to disabled before sleeping
    delayMs(100);
    enterLowPowerMode();
}
