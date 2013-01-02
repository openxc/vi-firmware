#include <check.h>
#include <stdint.h>
#include "buffers.h"

ByteQueue queue;
bool called;

void setup() {
    QUEUE_INIT(uint8_t, &queue);
    called = false;
}

void teardown() {
}

bool callback(uint8_t* message) {
    called = true;
}

START_TEST (test_empty_doesnt_call)
{
    processQueue(&queue, callback);
    fail_if(called);
}
END_TEST

Suite* buffersSuite(void) {
    Suite* s = suite_create("buffers");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture (tc_core, setup, teardown);
    tcase_add_test(tc_core, test_empty_doesnt_call);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = buffersSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
