#include <check.h>
#include <stdint.h>
#include "canutil.h"
#include "canread.h"
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

const int SIGNAL_COUNT = 5;
CanSignal SIGNALS[SIGNAL_COUNT] = {
    {&MESSAGES[0], "torque_at_transmission", 2, 4, 1001.0, -30000.000000, -5000.000000,
        33522.000000, 1, false, false, NULL, 0, true},
    {&MESSAGES[1], "transmission_gear_position", 1, 3, 1.000000, 0.000000, 0.000000,
        0.000000, 1, false, false, SIGNAL_STATES[0], 6, true, NULL, 4.0},
    {&MESSAGES[2], "brake_pedal_status", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000, 1,
        false, false, NULL, 0, true},
    {&MESSAGES[2], "command", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000, 1,
        false, true, NULL, 0, false},
    {&MESSAGES[2], "command", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000, 1,
        false, true, NULL, 0, true},
};

const int COMMAND_COUNT = 1;
CanCommand COMMANDS[COMMAND_COUNT] = {
    {"turn_signal_status", NULL},
};

START_TEST (test_can_signal_struct)
{
    CanSignal signal = SIGNALS[0];
    ck_assert_int_eq(signal.message->id, 0);
    ck_assert_str_eq(signal.genericName, "torque_at_transmission");
    ck_assert_int_eq(signal.bitPosition, 2);
    ck_assert_int_eq(signal.bitSize, 4);
    ck_assert_int_eq(signal.factor, 1001.0);
    ck_assert_int_eq(signal.offset, -30000.0);
    ck_assert_int_eq(signal.minValue, -5000.0);
    ck_assert_int_eq(signal.maxValue, 33522.0);

    signal = SIGNALS[1];
    ck_assert_int_eq(signal.lastValue, 4.0);
}
END_TEST

START_TEST (test_can_signal_states)
{
    CanSignal signal = SIGNALS[1];
    ck_assert_int_eq(signal.message->id, 1);
    ck_assert_int_eq(signal.stateCount, 6);
    ck_assert_int_eq(signal.states[0].value, 1);
    ck_assert_str_eq(signal.states[0].name, "reverse");
}
END_TEST

START_TEST (test_lookup_signal)
{
    fail_unless(lookupSignal("does_not_exist", SIGNALS, SIGNAL_COUNT) == NULL);
    fail_unless(lookupSignal("torque_at_transmission", SIGNALS, SIGNAL_COUNT)
            == &SIGNALS[0]);
    fail_unless(lookupSignal("transmission_gear_position", SIGNALS,
            SIGNAL_COUNT) == &SIGNALS[1]);
}
END_TEST

START_TEST (test_lookup_writable_signal)
{
    fail_unless(lookupSignal("does_not_exist", SIGNALS, SIGNAL_COUNT, true) == NULL);
    fail_unless(lookupSignal("transmission_gear_position", SIGNALS,
            SIGNAL_COUNT, false) == &SIGNALS[1]);
    fail_unless(lookupSignal("command", SIGNALS,
            SIGNAL_COUNT, false) == &SIGNALS[3]);
    fail_unless(lookupSignal("command", SIGNALS,
            SIGNAL_COUNT, true) == &SIGNALS[4]);
}
END_TEST

START_TEST (test_lookup_signal_state_by_name)
{
    fail_unless(lookupSignalState("does_not_exist", &SIGNALS[1], SIGNALS,
                SIGNAL_COUNT) == NULL);
    fail_unless(lookupSignalState("reverse", &SIGNALS[1], SIGNALS, SIGNAL_COUNT)
            == &SIGNAL_STATES[0][0]);
    fail_unless(lookupSignalState("third", &SIGNALS[1], SIGNALS, SIGNAL_COUNT)
            == &SIGNAL_STATES[0][1]);
}
END_TEST

START_TEST (test_lookup_signal_state_by_value)
{
    fail_unless(lookupSignalState(42, &SIGNALS[1], SIGNALS,
                SIGNAL_COUNT) == NULL);
    fail_unless(lookupSignalState(1, &SIGNALS[1], SIGNALS, SIGNAL_COUNT)
            == &SIGNAL_STATES[0][0]);
    fail_unless(lookupSignalState(2, &SIGNALS[1], SIGNALS, SIGNAL_COUNT)
            == &SIGNAL_STATES[0][1]);
}
END_TEST

START_TEST (test_lookup_command)
{
    fail_unless(lookupCommand("does_not_exist", COMMANDS, COMMAND_COUNT) == NULL);
    fail_unless(lookupCommand("turn_signal_status", COMMANDS, COMMAND_COUNT)
            == &COMMANDS[0]);
}
END_TEST

START_TEST (test_initialize)
{
    CanBus bus = {500, 0x101};
    initializeCanCommon(&bus);
}
END_TEST

Suite* canutilSuite(void) {
    Suite* s = suite_create("canutil");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_initialize);
    tcase_add_test(tc_core, test_can_signal_struct);
    tcase_add_test(tc_core, test_can_signal_states);
    tcase_add_test(tc_core, test_lookup_signal);
    tcase_add_test(tc_core, test_lookup_writable_signal);
    tcase_add_test(tc_core, test_lookup_signal_state_by_name);
    tcase_add_test(tc_core, test_lookup_signal_state_by_value);
    tcase_add_test(tc_core, test_lookup_command);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = canutilSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
