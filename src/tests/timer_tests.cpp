#include <check.h>
#include <stdint.h>

#include "util/timer.h"

using openxc::util::time::systemTimeMs;
using openxc::util::time::FrequencyClock;
using openxc::util::time::tick;

void setup() {
}

void teardown() {
}

START_TEST (test_first_tick_always_true)
{
    FrequencyClock clock;
    initializeClock(&clock);
    clock.frequency = 1;
    ck_assert(conditionalTick(&clock));
}
END_TEST

START_TEST (test_no_time_function_uses_default)
{
    FrequencyClock clock;
    // the default is the system timer, which for the test suite is a mock timer
    // that never increases.
    initializeClock(&clock);
    clock.frequency = 1;
    ck_assert(conditionalTick(&clock));
    ck_assert(!conditionalTick(&clock));
    ck_assert(!conditionalTick(&clock));
}
END_TEST

START_TEST (test_zero_frequency_always_ticks)
{
    FrequencyClock clock;
    initializeClock(&clock);
    clock.frequency = 0;
    ck_assert(conditionalTick(&clock));
    ck_assert(conditionalTick(&clock));
    ck_assert(conditionalTick(&clock));
}
END_TEST

static unsigned long fakeTime = 1000;

unsigned long timeMock() {
    return fakeTime;
}

START_TEST (test_non_zero_frequency_waits)
{
    FrequencyClock clock;
    initializeClock(&clock);
    clock.timeFunction = timeMock;
    clock.frequency = 1;
    ck_assert(conditionalTick(&clock));
    ck_assert(!conditionalTick(&clock));
    fakeTime += 1000;
    ck_assert(conditionalTick(&clock));
}
END_TEST

START_TEST (test_staggered_not_true_at_start)
{
    FrequencyClock clock;
    initializeClock(&clock);
    clock.timeFunction = timeMock;
    clock.frequency = 1;
    ck_assert(!conditionalTick(&clock, true));
    fakeTime += 1000;
    ck_assert(conditionalTick(&clock));
}
END_TEST

START_TEST (test_nonconditional_tick)
{
    FrequencyClock clock;
    initializeClock(&clock);
    clock.timeFunction = timeMock;
    clock.frequency = 1;
    ck_assert(conditionalTick(&clock));

    fakeTime += 800;
    ck_assert(!conditionalTick(&clock));

    fakeTime += 200;
    ck_assert(conditionalTick(&clock));

    fakeTime += 200;
    tick(&clock);
    ck_assert(!conditionalTick(&clock));
    fakeTime += 800;
    ck_assert(!conditionalTick(&clock));
    fakeTime += 200;
    ck_assert(conditionalTick(&clock));
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("timer");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture (tc_core, setup, teardown);
    tcase_add_test(tc_core, test_no_time_function_uses_default);
    tcase_add_test(tc_core, test_zero_frequency_always_ticks);
    tcase_add_test(tc_core, test_non_zero_frequency_waits);
    tcase_add_test(tc_core, test_first_tick_always_true);
    tcase_add_test(tc_core, test_staggered_not_true_at_start);
    tcase_add_test(tc_core, test_nonconditional_tick);
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
