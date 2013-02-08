#include <check.h>
#include <stdint.h>
#include "shared_handlers.h"
#include "canwrite.h"

CanMessage MESSAGES[3] = {
    {NULL, 0},
    {NULL, 1},
};

const int SIGNAL_COUNT = 7;

CanSignalState SIGNAL_STATES[SIGNAL_COUNT][12] = {
    { {1, "right"}, {2, "down"}, {3, "left"}, {4, "ok"}, {5, "up"}, },
    { {1, "idle"}, {2, "stuck"}, {3, "held_short"}, {4, "pressed"}, {5, "held_long"},
        {6, "released"}, },
};

CanSignal SIGNALS[SIGNAL_COUNT] = {
    {&MESSAGES[0], "button_type", 8, 8, 1.000000, 0.000000, 0.000000,
        0.000000, 1, true, false, SIGNAL_STATES[0], 5, false, NULL},
    {&MESSAGES[0], "button_state", 20, 4, 1.000000, 0.000000, 0.000000,
        0.000000, 1, true, false, SIGNAL_STATES[1], 6, false, NULL},
    {&MESSAGES[1], "driver_door", 15, 1, 1.000000, 0.000000, 0.000000,
        0.000000, 1, false, false, NULL, 0, false, NULL},
    {&MESSAGES[1], "passenger_door", 16, 1, 1.000000, 0.000000, 0.000000,
        0.000000, 1, false, false, NULL, 0, false, NULL},
    {&MESSAGES[1], "rear_left_door", 17, 1, 1.000000, 0.000000, 0.000000,
        0.000000, 1, false, false, NULL, 0, false, NULL},
    {&MESSAGES[1], "rear_right_door", 18, 1, 1.000000, 0.000000, 0.000000,
        0.000000, 1, false, false, NULL, 0, false, NULL},
    {&MESSAGES[1], "fuel_consumed_since_restart", 18, 1, 25.000000, 0.000000, 0.000000,
        255.0, 1, false, false, NULL, 0, false, NULL},
};

Listener listener;
UsbDevice usb;

void setup() {
    listener.usb = &usb;
    initializeUsb(&usb);
    listener.usb->configured = true;
    for(int i = 0; i < SIGNAL_COUNT; i++) {
        SIGNALS[i].received = false;
        SIGNALS[i].sendFrequency = 1;
        SIGNALS[i].sendClock = 0;
    }
}

START_TEST (test_button_event_handler)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    bool send = true;
    uint64_t data =  stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "down",
            &send);
    data = stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, "stuck",
            &send, data);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

START_TEST (test_button_event_handler_bad_type)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    bool send = true;
    uint64_t data =  stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "bad",
            &send);
    data = stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, "stuck",
            &send, data);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &listener);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

START_TEST (test_button_event_handler_correct_types)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    bool send = true;
    uint64_t data =  stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "down",
            &send);
    data = stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, "stuck",
            &send, data);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "event") == NULL);
    fail_if(strstr((char*)snapshot, "value") == NULL);
    fail_if(strstr((char*)snapshot, "stuck") == NULL);
    fail_if(strstr((char*)snapshot, "down") == NULL);
}
END_TEST

START_TEST (test_button_event_handler_bad_state)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    bool send = true;
    uint64_t data = stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "down",
            &send);
    data = numberWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, 11, &send, data);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &listener);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

START_TEST (test_door_handler)
{
    bool send = true;
    uint64_t data = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT, true,
            &send);
    sendDoorStatus("foo", data, &SIGNALS[2], SIGNALS, SIGNAL_COUNT, &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "foo") == NULL);
}
END_TEST

START_TEST (test_send_invalid_door_status)
{
    bool send = true;
    uint64_t data = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT, true,
            &send);
    sendDoorStatus("does-not-exist", data, NULL, SIGNALS, SIGNAL_COUNT, &listener);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

START_TEST (test_send_same_door_status)
{
    bool send = true;
    uint64_t data = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT, true,
            &send);
    sendDoorStatus("driver", data, &SIGNALS[2], SIGNALS, SIGNAL_COUNT, &listener);
    fail_if(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    QUEUE_INIT(uint8_t, &listener.usb->sendQueue);
    sendDoorStatus("driver", data, &SIGNALS[2], SIGNALS, SIGNAL_COUNT, &listener);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

START_TEST (test_fuel_handler)
{
    bool send = true;
    float result = 0;
    CanSignal* signal = &SIGNALS[6];
    result = handleFuelFlow(signal, SIGNALS, SIGNAL_COUNT, 0, &send, 1);
    ck_assert_int_eq(0, result);
    signal->lastValue = 0;

    result = handleFuelFlow(signal, SIGNALS, SIGNAL_COUNT, 1, &send, 1);
    ck_assert_int_eq(1, result);
    signal->lastValue = 1;

    result = handleFuelFlow(signal, SIGNALS, SIGNAL_COUNT, 255, &send, 1);
    ck_assert_int_eq(255, result);
    signal->lastValue = 255;

    result = handleFuelFlow(signal, SIGNALS, SIGNAL_COUNT, 2, &send, 1);
    ck_assert_int_eq(257, result);
    signal->lastValue = 2;
}
END_TEST

Suite* handlerSuite(void) {
    Suite* s = suite_create("shared_handlers");
    TCase *tc_button_handler = tcase_create("button");
    tcase_add_checked_fixture(tc_button_handler, setup, NULL);
    tcase_add_test(tc_button_handler, test_button_event_handler);
    tcase_add_test(tc_button_handler, test_button_event_handler_bad_type);
    tcase_add_test(tc_button_handler, test_button_event_handler_bad_state);
    tcase_add_test(tc_button_handler, test_button_event_handler_correct_types);
    suite_add_tcase(s, tc_button_handler);

    TCase *tc_door_handler = tcase_create("door");
    tcase_add_checked_fixture(tc_door_handler, setup, NULL);
    tcase_add_test(tc_door_handler, test_door_handler);
    tcase_add_test(tc_door_handler, test_send_invalid_door_status);
    tcase_add_test(tc_door_handler, test_send_same_door_status);
    suite_add_tcase(s, tc_door_handler);

    TCase *tc_fuel_handler = tcase_create("fuel");
    tcase_add_checked_fixture(tc_fuel_handler, setup, NULL);
    tcase_add_test(tc_fuel_handler, test_fuel_handler);
    suite_add_tcase(s, tc_fuel_handler);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = handlerSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
