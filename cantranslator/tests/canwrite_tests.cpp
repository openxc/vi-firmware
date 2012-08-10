#include <check.h>
#include <stdint.h>
#include "helpers.h"
#include "canutil.h"
#include "canwrite.h"
#include "cJSON.h"

CanSignalState SIGNAL_STATES[1][10] = {
    { {1, "reverse"}, {2, "third"}, {3, "sixth"}, {4, "seventh"},
        {5, "neutral"}, {6, "second"}, },
};

const int SIGNAL_COUNT = 3;
CanSignal SIGNALS[SIGNAL_COUNT] = {
    {NULL, 0, "torque_at_transmission", 2, 4, 1001.0, -30000.000000, -5000.000000,
        33522.000000, 1, false, false, NULL, 0, true},
    {NULL, 1, "transmission_gear_position", 1, 3, 1.000000, 0.000000, 0.000000,
        0.000000, 1, false, false, SIGNAL_STATES[0], 6, true, NULL, 4.0},
    {NULL, 2, "brake_pedal_status", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000, 1,
        false, false, NULL, 0, true},
};

const int COMMAND_COUNT = 1;
CanCommand COMMANDS[COMMAND_COUNT] = {
    {"turn_signal_status", NULL},
};

START_TEST (test_number_writer)
{
    bool send = true;
    uint64_t value = numberWriter(&SIGNALS[0], SIGNALS,
            SIGNAL_COUNT, cJSON_CreateNumber(0xa), &send);
    check_equal_unit64(value, 0x7400000000000000);
    fail_unless(send);

    value = numberWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateNumber(0x6), &send);
    check_equal_unit64(value, 0x6000000000000000);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_writer)
{
    bool send = true;
    uint64_t value = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateNumber(true), &send);
    check_equal_unit64(value, 0x8000000000000000);
    fail_unless(send);
}
END_TEST

START_TEST (test_state_writer)
{
    bool send = true;
    uint64_t value = stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateString(SIGNAL_STATES[0][1].name), &send);
    check_equal_unit64(value, 0x2000000000000000);
    fail_unless(send);
}
END_TEST

START_TEST (test_write_unknown_state)
{
    bool send = true;
    stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateString("not_a_state"), &send);
    fail_unless(!send);
}
END_TEST

START_TEST (test_encode_can_signal)
{
    uint64_t value = encodeCanSignal(&SIGNALS[1], 0);
    check_equal_unit64(value, 0);
}
END_TEST

Suite* canwriteSuite(void) {
    Suite* s = suite_create("canwrite");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_number_writer);
    tcase_add_test(tc_core, test_boolean_writer);
    tcase_add_test(tc_core, test_state_writer);
    tcase_add_test(tc_core, test_write_unknown_state);
    tcase_add_test(tc_core, test_encode_can_signal);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = canwriteSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
