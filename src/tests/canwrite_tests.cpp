#include <check.h>
#include <stdint.h>
#include "canutil.h"
#include "canwrite.h"
#include "cJSON.h"

CanBus bus = {115200, 0x101};

CanMessage MESSAGES[3] = {
    {&bus, 0},
    {&bus, 1},
    {&bus, 2},
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
    QUEUE_INIT(CanMessage, &bus.sendQueue);
}

START_TEST (test_number_writer)
{
    bool send = true;
    uint64_t value = numberWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, 0xa,
            &send);
    ck_assert_int_eq(value, 0x1e00000000000000LLU);
    fail_unless(send);

    value = numberWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, 0x6, &send);
    ck_assert_int_eq(value, 0x6000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_writer)
{
    bool send = true;
    uint64_t value = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT, true,
            &send);
    ck_assert_int_eq(value, 0x8000000000000000LLU);
    fail_unless(send);

    value = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT, false, &send);
    ck_assert_int_eq(value, 0x0000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_state_writer)
{
    bool send = true;
    uint64_t value = stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateString(SIGNAL_STATES[0][1].name), &send);
    ck_assert_int_eq(value, 0x2000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_state_writer_null_string)
{
    bool send = true;
    uint64_t value = stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            (const char*)NULL, &send);
    fail_if(send);

    send = true;
    value = stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            (cJSON*)NULL, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_write_not_allowed)
{
    bool send = true;
    SIGNALS[1].writable = false;
    numberWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, 0x6, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_write_unknown_state)
{
    bool send = true;
    stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT,
            cJSON_CreateString("not_a_state"), &send);
    fail_if(send);
}
END_TEST

START_TEST (test_encode_can_signal)
{
    uint64_t value = encodeCanSignal(&SIGNALS[1], 0);
    ck_assert_int_eq(value, 0);
}
END_TEST

START_TEST (test_encode_can_signal_rounding_precision)
{
    uint64_t value = encodeCanSignal(&SIGNALS[3], 50);
    ck_assert_int_eq(value, 0x061a800000000000LLU);
}
END_TEST

START_TEST (test_enqueue)
{
    CanMessage message = {&bus, 42};
    enqueueCanMessage(&message, 0x123456);

    ck_assert_int_eq(1, QUEUE_LENGTH(CanMessage, &message.bus->sendQueue));
}
END_TEST

START_TEST (test_swaps_byte_order)
{
    CanMessage message = {&bus, 42};
    enqueueCanMessage(&message, 0x123456);

    CanMessage queuedMessage = QUEUE_POP(CanMessage, &message.bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data, 0x5634120000000000LLU);
}
END_TEST

START_TEST (test_send_with_null_writer)
{
    fail_unless(sendCanSignal(&SIGNALS[0], cJSON_CreateNumber(0xa),
                NULL, SIGNALS, SIGNAL_COUNT));
    CanMessage queuedMessage = QUEUE_POP(CanMessage, &SIGNALS[0].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data, 0x1e);
}
END_TEST

START_TEST (test_send_using_default)
{
    fail_unless(sendCanSignal(&SIGNALS[0], cJSON_CreateNumber(0xa), SIGNALS,
                SIGNAL_COUNT));
    CanMessage queuedMessage = QUEUE_POP(CanMessage, &SIGNALS[0].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data, 0x1e);
}
END_TEST

START_TEST (test_send_with_custom_with_states)
{
    fail_unless(sendCanSignal(&SIGNALS[1],
                cJSON_CreateString(SIGNAL_STATES[0][1].name), SIGNALS,
                SIGNAL_COUNT));
    CanMessage queuedMessage = QUEUE_POP(CanMessage, &SIGNALS[1].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data, 0x20);
}
END_TEST

uint64_t customStateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, cJSON* value, bool* send) {
    *send = false;
    return 0;
}

START_TEST (test_send_with_custom_says_no_send)
{
    fail_if(sendCanSignal(&SIGNALS[1],
                cJSON_CreateString(SIGNAL_STATES[0][1].name), customStateWriter,
                SIGNALS, SIGNAL_COUNT));
    fail_unless(QUEUE_EMPTY(CanMessage, &SIGNALS[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_force_send)
{
    fail_if(sendCanSignal(&SIGNALS[1],
                cJSON_CreateString(SIGNAL_STATES[0][1].name), customStateWriter,
                SIGNALS, SIGNAL_COUNT, true));
    ck_assert_int_eq(1, QUEUE_LENGTH(CanMessage, &SIGNALS[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_write_empty)
{
    processCanWriteQueue(&bus);
}
END_TEST

START_TEST (test_no_write_handler)
{
    sendCanSignal(&SIGNALS[0], cJSON_CreateNumber(0xa), SIGNALS, SIGNAL_COUNT);
    bus.writeHandler = NULL;
    processCanWriteQueue(&bus);
    fail_unless(QUEUE_EMPTY(CanMessage, &SIGNALS[1].message->bus->sendQueue));
}
END_TEST

bool writeHandler(CanBus* bus, CanMessage message) {
    return false;
}

START_TEST (test_failed_write_handler)
{
    sendCanSignal(&SIGNALS[0], cJSON_CreateNumber(0xa), SIGNALS, SIGNAL_COUNT);
    bus.writeHandler = writeHandler;
    processCanWriteQueue(&bus);
    fail_unless(QUEUE_EMPTY(CanMessage, &SIGNALS[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_successful_write)
{
    sendCanSignal(&SIGNALS[0], cJSON_CreateNumber(0xa), SIGNALS, SIGNAL_COUNT);
    processCanWriteQueue(&bus);
    fail_unless(QUEUE_EMPTY(CanMessage, &SIGNALS[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_write_multiples)
{
    sendCanSignal(&SIGNALS[0], cJSON_CreateNumber(0xa), SIGNALS, SIGNAL_COUNT);
    sendCanSignal(&SIGNALS[0], cJSON_CreateNumber(0xa), SIGNALS, SIGNAL_COUNT);
    processCanWriteQueue(&bus);
    fail_unless(QUEUE_EMPTY(CanMessage, &SIGNALS[1].message->bus->sendQueue));
}
END_TEST

Suite* canwriteSuite(void) {
    Suite* s = suite_create("canwrite");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_number_writer);
    tcase_add_test(tc_core, test_boolean_writer);
    tcase_add_test(tc_core, test_state_writer);
    tcase_add_test(tc_core, test_state_writer_null_string);
    tcase_add_test(tc_core, test_write_not_allowed);
    tcase_add_test(tc_core, test_write_unknown_state);
    tcase_add_test(tc_core, test_encode_can_signal);
    tcase_add_test(tc_core, test_encode_can_signal_rounding_precision);
    suite_add_tcase(s, tc_core);

    TCase *tc_enqueue = tcase_create("enqueue");
    tcase_add_checked_fixture(tc_enqueue, setup, NULL);
    tcase_add_test(tc_enqueue, test_enqueue);
    tcase_add_test(tc_enqueue, test_swaps_byte_order);
    tcase_add_test(tc_enqueue, test_send_using_default);
    tcase_add_test(tc_enqueue, test_send_with_null_writer);
    tcase_add_test(tc_enqueue, test_send_with_custom_with_states);
    tcase_add_test(tc_enqueue, test_send_with_custom_says_no_send);
    tcase_add_test(tc_enqueue, test_force_send);
    suite_add_tcase(s, tc_enqueue);

    TCase *tc_write = tcase_create("write");
    tcase_add_checked_fixture(tc_write, setup, NULL);
    tcase_add_test(tc_write, test_write_empty);
    tcase_add_test(tc_write, test_no_write_handler);
    tcase_add_test(tc_write, test_failed_write_handler);
    tcase_add_test(tc_write, test_successful_write);
    tcase_add_test(tc_write, test_write_multiples);
    suite_add_tcase(s, tc_write);

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
