#include <check.h>
#include <stdint.h>
#include "shared_handlers.h"
#include "canwrite.h"

CanMessage MESSAGES[3] = {
    {NULL, 0},
};

const int SIGNAL_COUNT = 3;

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

START_TEST (test_button_event_handler_bad_state)
{
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
    bool send = true;
    uint64_t data =  stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "down",
            &send);
    data = numberWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateNumber(42), &send, data);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &listener);
    fail_unless(QUEUE_EMPTY(uint8_t, &listener.usb->sendQueue));
}
END_TEST

Suite* handlerSuite(void) {
    Suite* s = suite_create("shared_handlers");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_button_event_handler);
    tcase_add_test(tc_core, test_button_event_handler_bad_type);
    tcase_add_test(tc_core, test_button_event_handler_bad_state);
    suite_add_tcase(s, tc_core);

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
