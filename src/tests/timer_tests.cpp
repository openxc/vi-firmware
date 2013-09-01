#include <check.h>
#include <stdint.h>

#include "util/timer.h"

using openxc::util::time::systemTimeMs;
using openxc::util::time::FrequencyClock;
using openxc::util::time::shouldTick;

void setup() {
}

void teardown() {
}

START_TEST (test_first_tick_always_true)
{
    FrequencyClock clock;
    initializeClock(&clock);
    clock.frequency = 1;
    ck_assert(shouldTick(&clock));
}
END_TEST

START_TEST (test_no_time_function_uses_default)
{
    FrequencyClock clock;
    // the default is the system timer, which for the test suite is a mock timer
    // that never increases.
    initializeClock(&clock);
    clock.frequency = 1;
    ck_assert(shouldTick(&clock));
    ck_assert(!shouldTick(&clock));
    ck_assert(!shouldTick(&clock));
}
END_TEST

START_TEST (test_zero_frequency_always_ticks)
{
    FrequencyClock clock;
    initializeClock(&clock);
    clock.frequency = 0;
    ck_assert(shouldTick(&clock));
    ck_assert(shouldTick(&clock));
    ck_assert(shouldTick(&clock));
}
END_TEST

static unsigned long fakeTime = 0;

unsigned long timeMock() {
    return fakeTime;
}

START_TEST (test_non_zero_frequency_waits)
{
    FrequencyClock clock;
    initializeClock(&clock);
    clock.timeFunction = timeMock;
    clock.frequency = 1;
    ck_assert(shouldTick(&clock));
    ck_assert(!shouldTick(&clock));
    fakeTime = 1000;
    ck_assert(shouldTick(&clock));
}
END_TEST

Suite* buffersSuite(void) {
    Suite* s = suite_create("buffers");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture (tc_core, setup, teardown);
    tcase_add_test(tc_core, test_no_time_function_uses_default);
    tcase_add_test(tc_core, test_zero_frequency_always_ticks);
    tcase_add_test(tc_core, test_non_zero_frequency_waits);
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
