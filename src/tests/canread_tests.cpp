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

const int SIGNAL_COUNT = 3;
CanSignal SIGNALS[SIGNAL_COUNT] = {
    {&MESSAGES[0], "torque_at_transmission", 2, 4, 1001.0, -30000.000000, -5000.000000,
        33522.000000, 1, false, false, NULL, 0, true},
    {&MESSAGES[1], "transmission_gear_position", 1, 3, 1.000000, 0.000000, 0.000000,
        0.000000, 1, false, false, SIGNAL_STATES[0], 6, true, NULL, 4.0},
    {&MESSAGES[2], "brake_pedal_status", 0, 1, 1.000000, 0.000000, 0.000000, 0.000000, 1,
        false, false, NULL, 0, true},
};

const int COMMAND_COUNT = 1;
CanCommand COMMANDS[COMMAND_COUNT] = {
    {"turn_signal_status", NULL},
};

Listener listener;
UsbDevice usb;

void setup() {
    listener.usb = &usb;
    initializeUsb(&usb);
    listener.usb->configured = true;
    for(int i = 0; i < SIGNAL_COUNT; i++) {
        SIGNALS[i].received = false;
        SIGNALS[i].sendSame = true;
        SIGNALS[i].sendFrequency = 1;
        SIGNALS[i].sendClock = 0;
    }
}

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
    ck_assert_int_eq(passthroughHandler(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, 42.0, &send), 42.0);
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
    fail_if(booleanHandler(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, 0, &send));
    fail_unless(send);
}
END_TEST

START_TEST (test_ignore_handler)
{
    bool send = true;
    ignoreHandler(&SIGNALS[0], SIGNALS, 2, 1.0, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_state_handler)
{
    bool send = true;
    ck_assert_str_eq(stateHandler(&SIGNALS[1], SIGNALS, 2, 2, &send),
            SIGNAL_STATES[0][1].name);
    fail_unless(send);
    stateHandler(&SIGNALS[1], SIGNALS, 2, 42, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_send_numerical)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    sendNumericalMessage("test", 42, &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"test\",\"value\":42}\r\n");
}
END_TEST

START_TEST (test_preserve_float_precision)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    float value = 42.5;
    sendNumericalMessage("test", value, &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"test\",\"value\":42.500000}\r\n");
}
END_TEST

START_TEST (test_send_boolean)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    sendBooleanMessage("test", false, &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"test\",\"value\":false}\r\n");
}
END_TEST

START_TEST (test_send_string)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    sendStringMessage("test", "string", &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"test\",\"value\":\"string\"}\r\n");
}
END_TEST

START_TEST (test_send_evented_boolean)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    sendEventedBooleanMessage("test", "value", false, &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"test\",\"value\":\"value\",\"event\":false}\r\n");
}
END_TEST

START_TEST (test_send_evented_string)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    sendEventedStringMessage("test", "value", "event", &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"test\",\"value\":\"value\",\"event\":\"event\"}\r\n");
}
END_TEST

START_TEST (test_passthrough_message)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    passthroughCanMessage(&listener, 42, 0x123456789ABCDEF1LLU);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"id\":42,\"data\":\"0xf1debc9a78563412\"}\r\n");
}
END_TEST

float floatHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return 42;
}

START_TEST (test_default_handler)
{
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"torque_at_transmission\",\"value\":-19990}\r\n");
}
END_TEST

const char* noSendStringHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    *send = false;
    return NULL;
}

bool noSendBooleanTranslateHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    *send = false;
    return false;
}

START_TEST (test_translate_respects_send_value)
{
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, ignoreHandler, SIGNALS,
            SIGNAL_COUNT);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    translateCanSignal(&listener, &SIGNALS[0], 0xEB, noSendStringHandler, SIGNALS,
            SIGNAL_COUNT);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    translateCanSignal(&listener, &SIGNALS[0], 0xEB,
            noSendBooleanTranslateHandler, SIGNALS, SIGNAL_COUNT);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

START_TEST (test_translate_float)
{
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, floatHandler, SIGNALS,
            SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"torque_at_transmission\",\"value\":42}\r\n");
}
END_TEST

const char* stringHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    return "foo";
}

START_TEST (test_translate_string)
{
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, stringHandler, SIGNALS,
            SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"torque_at_transmission\",\"value\":\"foo\"}\r\n");
}
END_TEST

bool booleanTranslateHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    return false;
}

START_TEST (test_translate_bool)
{
    translateCanSignal(&listener, &SIGNALS[2], 0xEB, booleanTranslateHandler, SIGNALS,
            SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"brake_pedal_status\",\"value\":false}\r\n");
}
END_TEST

START_TEST (test_always_send_first)
{
    SIGNALS[0].sendFrequency = 5;
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

START_TEST (test_limited_frequency)
{
    SIGNALS[0].sendFrequency = 5;
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    QUEUE_INIT(uint8_t, &listener.usb->sendQueue);
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

float preserveHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return signal->lastValue;
}

START_TEST (test_preserve_last_value)
{
    translateCanSignal(&listener, &SIGNALS[0], 0xEB, SIGNALS, SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    QUEUE_INIT(uint8_t, &listener.usb->sendQueue);

    translateCanSignal(&listener, &SIGNALS[0], 0x1234123, preserveHandler, SIGNALS,
            SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"torque_at_transmission\",\"value\":-19990}\r\n");
}
END_TEST

START_TEST (test_dont_send_same)
{
    SIGNALS[2].sendSame = false;
    translateCanSignal(&listener, &SIGNALS[2], 0xEB, booleanHandler, SIGNALS,
            SIGNAL_COUNT);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"brake_pedal_status\",\"value\":true}\r\n");

    QUEUE_INIT(uint8_t, &listener.usb->sendQueue);
    translateCanSignal(&listener, &SIGNALS[2], 0xEB, booleanHandler, SIGNALS,
            SIGNAL_COUNT);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

Suite* canreadSuite(void) {
    Suite* s = suite_create("canread");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_decode_signal);
    tcase_add_test(tc_core, test_passthrough_handler);
    tcase_add_test(tc_core, test_boolean_handler);
    tcase_add_test(tc_core, test_ignore_handler);
    tcase_add_test(tc_core, test_state_handler);
    suite_add_tcase(s, tc_core);

    TCase *tc_sending = tcase_create("sending");
    tcase_add_checked_fixture(tc_sending, setup, NULL);
    tcase_add_test(tc_sending, test_send_numerical);
    tcase_add_test(tc_sending, test_preserve_float_precision);
    tcase_add_test(tc_sending, test_send_boolean);
    tcase_add_test(tc_sending, test_send_string);
    tcase_add_test(tc_sending, test_send_evented_boolean);
    tcase_add_test(tc_sending, test_send_evented_string);
    tcase_add_test(tc_sending, test_passthrough_message);
    suite_add_tcase(s, tc_sending);

    TCase *tc_translate = tcase_create("translate");
    tcase_add_checked_fixture(tc_translate, setup, NULL);
    tcase_add_test(tc_translate, test_translate_float);
    tcase_add_test(tc_translate, test_translate_string);
    tcase_add_test(tc_translate, test_translate_bool);
    tcase_add_test(tc_translate, test_limited_frequency);
    tcase_add_test(tc_translate, test_always_send_first);
    tcase_add_test(tc_translate, test_preserve_last_value);
    tcase_add_test(tc_translate, test_default_handler);
    tcase_add_test(tc_translate, test_dont_send_same);
    tcase_add_test(tc_translate, test_translate_respects_send_value);
    suite_add_tcase(s, tc_translate);

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
