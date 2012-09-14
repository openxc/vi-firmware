#include <check.h>
#include <stdint.h>
#include "canutil.h"
#include "canread.h"
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

START_TEST (test_decode_signal)
{
    CanSignal signal = SIGNALS[0];
    uint64_t data = 0xEB;
    float result = decodeCanSignal(&signal, data);
    float correctResult = 0xA * 1001.0 - 30000.0;
    fail_unless(result == correctResult,
            "decode is incorrect: %f but should be %f", result, correctResult);
}
END_TEST

START_TEST (test_passthrough_handler)
{
    bool send = true;
    fail_unless(passthroughHandler(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, 42.0, &send) == 42.0);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_handler)
{
    bool send = true;
    fail_unless(booleanHandler(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, 1.0, &send));
    fail_unless(send);
    fail_unless(booleanHandler(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, 0.5, &send));
    fail_unless(send);
    fail_unless(!booleanHandler(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, 0, &send));
    fail_unless(send);
}
END_TEST

START_TEST (test_ignore_handler)
{
    bool send = true;
    ignoreHandler(&SIGNALS[0], SIGNALS, 2, 1.0, &send);
    fail_unless(!send);
}
END_TEST

START_TEST (test_state_handler)
{
    bool send = true;
    fail_unless(strcmp(stateHandler(&SIGNALS[1], SIGNALS, 2, 2, &send),
            SIGNAL_STATES[0][1].name) == 0);
    fail_unless(send);
    stateHandler(&SIGNALS[1], SIGNALS, 2, 42, &send);
    fail_unless(!send);
}
END_TEST

Suite* canreadSuite(void) {
    Suite* s = suite_create("canread");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_decode_signal);
    tcase_add_test(tc_core, test_passthrough_handler);
    tcase_add_test(tc_core, test_boolean_handler);
    tcase_add_test(tc_core, test_ignore_handler);
    tcase_add_test(tc_core, test_state_handler);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = canreadSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
