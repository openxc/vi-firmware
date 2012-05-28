#include <check.h>
#include <stdint.h>
#include "canutil.h"

CanSignalState SIGNAL_STATES[2][10] = {
    { {1, "reverse"}, {2, "third"}, {3, "sixth"}, {4, "seventh"}, {5, "neutral"}, {6, "second"}, },
};

CanSignal SIGNALS[2] = {
    {0, "powertrain_torque", 2, 4, 1001.0, -30000.000000, -5000.000000, 33522.000000, 1, 0, false, false},
    {1, "transmission_gear_position", 1, 3, 1.000000, 0.000000, 0.000000, 0.000000, 1, 0, false, false, SIGNAL_STATES[0], 6, 4.0},
};

START_TEST (test_can_signal_struct)
{
    CanSignal signal = SIGNALS[0];
    fail_unless(signal.messageId == 0, "ID didn't match: %f", signal.messageId);
    fail_unless(strcmp(signal.genericName, "powertrain_torque") == 0,
            "generic name didn't match: %s", signal.genericName);
    fail_unless(signal.bitPosition == 2,
            "bit position didn't match: %f", signal.bitPosition);
    fail_unless(signal.bitSize == 4,
            "bit size didn't match: %f", signal.bitSize);
    fail_unless(signal.factor == 1001.0,
            "factor didn't match: %f", signal.factor);
    fail_unless(signal.offset == -30000.0,
            "offset didn't match: %f", signal.offset);
    fail_unless(signal.minValue == -5000.0,
            "min value didn't match: %f", signal.minValue);
    fail_unless(signal.maxValue == 33522.0,
            "max value didn't match: %f", signal.maxValue);

    signal = SIGNALS[1];
    fail_unless(signal.lastValue == 4.0, "last value didn't match");
}
END_TEST

START_TEST (test_can_signal_states)
{
    CanSignal signal = SIGNALS[1];
    fail_unless(signal.messageId == 1, "ID didn't match");
    fail_unless(signal.stateCount == 6, "state count didn't match");
    CanSignalState state = signal.states[0];
    fail_unless(state.value == 1, "state value didn't match");
    fail_unless(strcmp(state.name, "reverse") == 0, "state name didn't match");
}
END_TEST

START_TEST (test_lookup_signal)
{
    fail_unless(lookupSignal("does_not_exist", SIGNALS, 2) == 0);
    fail_unless(lookupSignal("powertrain_torque", SIGNALS, 2) == &SIGNALS[0]);
    fail_unless(lookupSignal("transmission_gear_position", SIGNALS, 2)
            == &SIGNALS[1]);
}
END_TEST

START_TEST (test_decode_signal)
{
    CanSignal signal = SIGNALS[0];
    uint8_t data = 0xEB;
    float result = decodeCanSignal(&signal, &data);
    float correctResult = 0xA * 1001.0 - 30000.0;
    fail_unless(result == correctResult,
            "decode is incorrect: %f but should be %f", result, correctResult);
}
END_TEST

START_TEST (test_passthrough_handler)
{
    bool send = true;
    fail_unless(passthroughHandler(&SIGNALS[0], SIGNALS, 2, 42.0, &send) == 42.0);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_handler)
{
    bool send = true;
    fail_unless(booleanHandler(&SIGNALS[0], SIGNALS, 2, 1.0, &send));
    fail_unless(send);
    fail_unless(booleanHandler(&SIGNALS[0], SIGNALS, 2, 0.5, &send));
    fail_unless(send);
    fail_unless(!booleanHandler(&SIGNALS[0], SIGNALS, 2, 0, &send));
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

START_TEST (test_passthrough_writer)
{
    bool send = true;
    uint32_t value = passthroughWriter(&SIGNALS[0], SIGNALS, 2, 0xa, &send);
    uint32_t expectedValue = 0x74000000;
    fail_unless(value == expectedValue, "Expected 0x%X but got 0x%X",
            expectedValue, value);
    fail_unless(send);

    value = passthroughWriter(&SIGNALS[1], SIGNALS, 2, 0x6, &send);
    expectedValue = 0x60000000;
    fail_unless(value == expectedValue, "Expected 0x%X but got 0x%X",
            expectedValue, value);
    fail_unless(send);
}
END_TEST

Suite* canutilSuite(void) {
    Suite* s = suite_create("canutil");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_can_signal_struct);
    tcase_add_test(tc_core, test_can_signal_states);
    tcase_add_test(tc_core, test_lookup_signal);
    tcase_add_test(tc_core, test_decode_signal);
    tcase_add_test(tc_core, test_passthrough_handler);
    tcase_add_test(tc_core, test_boolean_handler);
    tcase_add_test(tc_core, test_ignore_handler);
    tcase_add_test(tc_core, test_state_handler);
    tcase_add_test(tc_core, test_passthrough_writer);
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
