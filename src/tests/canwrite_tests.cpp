#include <check.h>
#include <stdint.h>
#include "helpers.h"
#include "canutil.h"
#include "canwrite.h"
#include "cJSON.h"

CanMessage MESSAGES[3] = {
    {NULL, 0},
    {NULL, 1},
    {NULL, 2},
};

CanSignalState SIGNAL_STATES[1][10] = {
    { {1, "reverse"}, {2, "third"}, {3, "sixth"}, {4, "seventh"},
        {5, "neutral"}, {6, "second"}, },
};

const int SIGNAL_COUNT = 4;
CanSignal SIGNALS[SIGNAL_COUNT] = {
    {&MESSAGES[0], "torque_at_transmission", 2, 6, 1001.0, -30000.000000, -5000.000000,
        33522.000000, 1, false, false, NULL, 0, true},
    {&MESSAGES[1], "transmission_gear_position", 1, 3, 1.000000, 0.000000, 0.000000,
        0.000000, 1, false, false, SIGNAL_STATES[0], 6, true, NULL, 4.0},
    {&MESSAGES[2], "brake_pedal_status", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000, 1,
        false, false, NULL, 0, true},
    {&MESSAGES[3], "measurement", 2, 19, 0.001000, 0.000000, 0, 500.0,
        0, false, false, SIGNAL_STATES[0], 6, true, NULL, 4.0},
};

const int COMMAND_COUNT = 1;
CanCommand COMMANDS[COMMAND_COUNT] = {
    {"turn_signal_status", NULL},
};

void setup() {
    for(int i = 0; i < SIGNAL_COUNT; i++) {
        SIGNALS[i].writable = true;
        SIGNALS[i].received = false;
        SIGNALS[i].sendSame = true;
        SIGNALS[i].sendFrequency = 1;
        SIGNALS[i].sendClock = 0;
    }
}

void teardown() {
}

START_TEST (test_number_writer)
{
    bool send = true;
    uint64_t value = numberWriter(&SIGNALS[0], SIGNALS,
            SIGNAL_COUNT, cJSON_CreateNumber(0xa), &send);
    check_equal_unit64(value, 0x1e00000000000000LLU);
    fail_unless(send);

    value = numberWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateNumber(0x6), &send);
    check_equal_unit64(value, 0x6000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_writer)
{
    bool send = true;
    uint64_t value = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateBool(true), &send);
    check_equal_unit64(value, 0x8000000000000000LLU);
    fail_unless(send);

    value = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateBool(false), &send);
    check_equal_unit64(value, 0x0000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_state_writer)
{
    bool send = true;
    uint64_t value = stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateString(SIGNAL_STATES[0][1].name), &send);
    check_equal_unit64(value, 0x2000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_write_not_allowed)
{
    bool send = true;
    SIGNALS[1].writable = false;
    numberWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, cJSON_CreateNumber(0x6),
            &send);
    fail_if(send);
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

START_TEST (test_encode_can_signal_rounding_precision)
{
    uint64_t value = encodeCanSignal(&SIGNALS[3], 50);
    check_equal_unit64(value, 0x061a800000000000LLU);
}
END_TEST

Suite* canwriteSuite(void) {
    Suite* s = suite_create("canwrite");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_number_writer);
    tcase_add_test(tc_core, test_boolean_writer);
    tcase_add_test(tc_core, test_state_writer);
    tcase_add_test(tc_core, test_write_not_allowed);
    tcase_add_test(tc_core, test_write_unknown_state);
    tcase_add_test(tc_core, test_encode_can_signal);
    tcase_add_test(tc_core, test_encode_can_signal_rounding_precision);
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
