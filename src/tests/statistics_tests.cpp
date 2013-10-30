#include <check.h>
#include <stdint.h>

#include "util/statistics.h"

using openxc::util::statistics::Statistic;
using openxc::util::statistics::DeltaStatistic;

namespace statistics = openxc::util::statistics;

void setup() {
}

void teardown() {
}

START_TEST (test_update_sets_min)
{
    Statistic stat;
    statistics::initialize(&stat);
    statistics::update(&stat, 1);
    statistics::update(&stat, 2);
    statistics::update(&stat, 3);
    ck_assert_int_eq(statistics::minimum(&stat), 1);
}
END_TEST

START_TEST (test_update_sets_max)
{
    Statistic stat;
    statistics::initialize(&stat);
    statistics::update(&stat, 1);
    statistics::update(&stat, 2);
    statistics::update(&stat, 3);
    statistics::update(&stat, 0);
    ck_assert_int_eq(statistics::maximum(&stat), 3);
}
END_TEST

START_TEST (test_average_starts_at_first_value)
{
    Statistic stat;
    statistics::initialize(&stat);
    statistics::update(&stat, 500);
    float average = statistics::exponentialMovingAverage(&stat);
    ck_assert(average == 500);
}
END_TEST

START_TEST (test_exponential_moving_average)
{
    Statistic stat;
    statistics::initialize(&stat);
    statistics::update(&stat, 1);
    statistics::update(&stat, 2);
    statistics::update(&stat, 3);
    statistics::update(&stat, 4);
    statistics::update(&stat, 5);
    statistics::update(&stat, 6);
    statistics::update(&stat, 7);
    statistics::update(&stat, 8);
    statistics::update(&stat, 9);
    statistics::update(&stat, 10);
    float average = statistics::exponentialMovingAverage(&stat);
    ck_assert(average > 4);
    ck_assert(average < 4.5);
}
END_TEST

START_TEST (test_delta_stat_min_max)
{
    DeltaStatistic stat;
    statistics::initialize(&stat);
    statistics::update(&stat, 1);
    statistics::update(&stat, 2);
    statistics::update(&stat, 10);
    ck_assert_int_eq(statistics::minimum(&stat), 1);
    ck_assert_int_eq(statistics::maximum(&stat), 8);
}
END_TEST

START_TEST (test_delta_stat_exponential_average)
{
    DeltaStatistic stat;
    statistics::initialize(&stat);
    for(int i = 0; i < 100; i++) {
        statistics::update(&stat, i * 5);
    }
    statistics::update(&stat, 506);
    float average = statistics::exponentialMovingAverage(&stat);
    ck_assert(average > 5);
    ck_assert(average < 6);
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("statistics");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture (tc_core, setup, teardown);
    tcase_add_test(tc_core, test_update_sets_min);
    tcase_add_test(tc_core, test_update_sets_max);
    tcase_add_test(tc_core, test_exponential_moving_average);
    tcase_add_test(tc_core, test_delta_stat_min_max);
    tcase_add_test(tc_core, test_delta_stat_exponential_average);
    tcase_add_test(tc_core, test_average_starts_at_first_value);
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
