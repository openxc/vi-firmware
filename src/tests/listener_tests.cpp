#include <check.h>
#include <stdint.h>
#include "listener.h"

ByteQueue queue;
bool called;
bool callbackStatus;

void setup() {
    QUEUE_INIT(uint8_t, &queue);
    called = false;
    callbackStatus = false;
}

void teardown() {
}

bool callback(uint8_t* message) {
    called = true;
    return callbackStatus;
}

Suite* listenerSuite(void) {
    Suite* s = suite_create("listener");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture (tc_core, setup, teardown);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = listenerSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
