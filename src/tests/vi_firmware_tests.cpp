#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "diagnostics.h"

namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;

using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;

// TODO this should be refactored out of vi_firmware.cpp, and include a header
// file so we don't have to use extern.
extern bool receiveWriteRequest(uint8_t*);

static bool canQueueEmpty(int bus) {
    return QUEUE_EMPTY(CanMessage, &getCanBuses()[bus].sendQueue);
}

void setup() {
    for(int i = 0; i < getCanBusCount(); i++) {
        openxc::can::initializeCommon(&getCanBuses()[i]);
    }
    fail_unless(canQueueEmpty(0));
}

const char* REQUEST = "{\"bus\": 1, \"id\": 42, \"data\": \"0x1234\"}";

START_TEST (test_raw_write_allowed)
{
    getCanBuses()[0].rawWritable = true;
    ck_assert(receiveWriteRequest((uint8_t*)REQUEST));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write_not_allowed)
{
    getCanBuses()[0].rawWritable = false;
    ck_assert(receiveWriteRequest((uint8_t*)REQUEST));
    fail_unless(canQueueEmpty(0));
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("firmware");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_raw_write_allowed);
    tcase_add_test(tc_core, test_raw_write_not_allowed);

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
