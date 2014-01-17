#include "platform/platform.h"

#include "power.h"
#include "lights.h"
#include "bluetooth.h"
#include "signals.h"
#include "util/timer.h"
#include "util/log.h"
#include "can/canread.h"

namespace usb = openxc::interface::usb;
namespace time = openxc::util::time;

using openxc::pipeline::Pipeline;
using openxc::util::log::debug;
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
    time::delayMs(100);
    power::suspend();
}
