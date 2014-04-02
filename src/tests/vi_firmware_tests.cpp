#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "diagnostics.h"
#include "lights.h"
#include "config.h"
#include "pipeline.h"
#include "power.h"

namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;

using openxc::pipeline::Pipeline;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::config::getConfiguration;

extern openxc::lights::RGB LIGHT_A_LAST_COLOR;
extern unsigned long FAKE_TIME;

// TODO this should be refactored out of vi_firmware.cpp, and include a header
// file so we don't have to use extern.
extern void receiveCan(Pipeline* pipeline, CanBus* bus);
extern void checkBusActivity();
extern void initializeVehicleInterface();
extern void firmwareLoop();


static bool canQueueEmpty(int bus) {
    return QUEUE_EMPTY(CanMessage, &getCanBuses()[bus].sendQueue);
}

void setup() {
    initializeVehicleInterface();
    fail_unless(canQueueEmpty(0));
}

CanMessage message = {
    id: 0x1,
    format: CanMessageFormat::STANDARD,
    data: {0x1, 0x2}
};

START_TEST (test_update_data_lights_can_active)
{
    CanBus* bus = &getCanBuses()[0];
    QUEUE_PUSH(CanMessage, &bus->receiveQueue, message);
    receiveCan(&getConfiguration()->pipeline, bus);

    checkBusActivity();
    ck_assert(openxc::lights::colors_equal(LIGHT_A_LAST_COLOR,
                openxc::lights::COLORS.blue));
}
END_TEST

START_TEST (test_update_data_lights_can_inactive)
{

    checkBusActivity();
    ck_assert(openxc::lights::colors_equal(LIGHT_A_LAST_COLOR,
                openxc::lights::COLORS.red));

    CanBus* bus = &getCanBuses()[0];
    QUEUE_PUSH(CanMessage, &bus->receiveQueue, message);
    receiveCan(&getConfiguration()->pipeline, bus);

    FAKE_TIME += (openxc::can::CAN_ACTIVE_TIMEOUT_S * 1000) * 2;

}
END_TEST

START_TEST (test_update_data_lights_suspend)
{
    CanBus* bus = &getCanBuses()[0];
    QUEUE_PUSH(CanMessage, &bus->receiveQueue, message);
    receiveCan(&getConfiguration()->pipeline, bus);

    FAKE_TIME += (openxc::can::CAN_ACTIVE_TIMEOUT_S * 1000) * 2;

    checkBusActivity();
    if(getConfiguration()->powerManagement == openxc::config::PowerManagement::ALWAYS_ON) {
        ck_assert(openxc::lights::colors_equal(LIGHT_A_LAST_COLOR,
                    openxc::lights::COLORS.red));
    } else {
        ck_assert(openxc::lights::colors_equal(LIGHT_A_LAST_COLOR,
                    openxc::lights::COLORS.black));
    }

}
END_TEST

START_TEST (test_loop)
{
    firmwareLoop();
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("firmware");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_update_data_lights_can_active);
    tcase_add_test(tc_core, test_update_data_lights_can_inactive);
    tcase_add_test(tc_core, test_update_data_lights_suspend);

    tcase_add_test(tc_core, test_loop);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = suite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
