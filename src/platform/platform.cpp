#include "platform/platform.h"

#include "power.h"
#include "lights.h"
#include "bluetooth.h"
#include "platform/pic32/telit_he910.h"
#include "platform/pic32/telit_he910_platforms.h"
#include "signals.h"
#include "config.h"
#include "util/timer.h"
#include "util/log.h"
#include "can/canread.h"

#define OBD2_IGNITION_CHECK_WATCHDOG_TIMEOUT_MICROSECONDS 15000000

namespace usb = openxc::interface::usb;
namespace time = openxc::util::time;
namespace telit = openxc::telitHE910;

using openxc::pipeline::Pipeline;
using openxc::util::log::debug;
using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;
using openxc::config::getConfiguration;
using openxc::config::PowerManagement;
using openxc::config::RunLevel;

void openxc::platform::suspend(Pipeline* pipeline) {
    debug("CAN went silent - disabling LED");

    // De-init and shut down all peripherals to save power
    for(int i = 0; i < getCanBusCount(); ++i) {
        can::deinitialize(&getCanBuses()[i]);
    }

    lights::deinitialize();
    usb::deinitialize(pipeline->usb);
    bluetooth::deinitialize();
    #ifdef TELIT_HE910_SUPPORT
    telit::deinitialize();
    #endif

    if(getConfiguration()->powerManagement == PowerManagement::OBD2_IGNITION_CHECK) {
        debug("Enabling watchdog timer to poll for ignition status via OBD-II");
        power::enableWatchdogTimer(OBD2_IGNITION_CHECK_WATCHDOG_TIMEOUT_MICROSECONDS);
    }

    // Wait for peripherals to disabled before sleeping
    time::delayMs(100);
    power::suspend();
}
