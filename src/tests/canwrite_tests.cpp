#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "can/canutil.h"
#include "can/canwrite.h"

namespace can = openxc::can;

using openxc::can::write::numberEncoder;
using openxc::can::write::stateEncoder;
using openxc::can::write::encodeSignal;
using openxc::signals::getSignalCount;
using openxc::signals::getSignals;
using openxc::signals::getCanBuses;

void setup() {
    for(int i = 0; i < getSignalCount(); i++) {
        getSignals()[i].writable = true;
        getSignals()[i].received = false;
        getSignals()[i].sendSame = true;
        getSignals()[i].frequencyClock = {0};
    }
    QUEUE_INIT(CanMessage, &getCanBuses()[0].sendQueue);
}

START_TEST (test_number_writer)
{
    bool send = true;
    uint64_t value = encodeSignal(&getSignals()[6], numberEncoder(&getSignals()[6], getSignals(),
            getSignalCount(), 0xa, &send));
    ck_assert_int_eq(value, 0x1e00000000000000LLU);
    fail_unless(send);

    value = encodeSignal(&getSignals()[1], numberEncoder(&getSignals()[1], getSignals(), getSignalCount(), 0x6,
            &send));
    ck_assert_int_eq(value, 0x6000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_writer)
{
    bool send = true;
    uint64_t value = encodeSignal(&getSignals()[2], numberEncoder(&getSignals()[2], getSignals(), getSignalCount(), float(true),
            &send));
    ck_assert_int_eq(value, 0x8000000000000000LLU);
    fail_unless(send);

    value = encodeSignal(&getSignals()[2], numberEncoder(&getSignals()[2], getSignals(), getSignalCount(), float(false), &send));
    ck_assert_int_eq(value, 0x0000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_state_writer)
{
    bool send = true;
    openxc_DynamicField field;
    field.has_string_value = true;
    strcpy(field.string_value, getSignals()[1].states[1].name);

    uint64_t value = encodeSignal(&getSignals()[1], stateEncoder(&getSignals()[1], getSignals(),
            getSignalCount(), &field, &send));
    ck_assert_int_eq(value, 0x2000000000000000LLU);
    fail_unless(send);
}
END_TEST

START_TEST (test_state_writer_null_string)
{
    bool send = true;
    uint64_t value = encodeSignal(&getSignals()[1], stateEncoder(&getSignals()[1], getSignals(), getSignalCount(),
            (const char*)NULL, &send));
    fail_if(send);

    send = true;
    value = encodeSignal(&getSignals()[1], stateEncoder(&getSignals()[1], getSignals(), getSignalCount(), (openxc_DynamicField*)NULL, &send));
    fail_if(send);
}
END_TEST

START_TEST (test_write_not_allowed)
{
    getSignals()[1].writable = false;
    openxc_DynamicField field;
    field.has_numeric_value = true;
    field.numeric_value = 0x6;

    can::write::sendSignal(&getSignals()[1], &field, getSignals(), getSignalCount());
    fail_unless(QUEUE_EMPTY(CanMessage, &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_write_unknown_state)
{
    bool send = true;
    openxc_DynamicField field;
    field.has_string_value = true;
    strcpy(field.string_value, "not_a_state");

    stateEncoder(&getSignals()[1], getSignals(), getSignalCount(), &field, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_enqueue)
{
    CanMessage message = {42, 0x123456};
    can::write::enqueueMessage(&getCanBuses()[0], &message);

    ck_assert_int_eq(1, QUEUE_LENGTH(CanMessage, &getCanBuses()[0].sendQueue));
}
END_TEST

START_TEST (test_swaps_byte_order)
{
    CanMessage message = {42, 0x123456};
    can::write::enqueueMessage(&getCanBuses()[0], &message);

    CanMessage queuedMessage = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(queuedMessage.data, 0x5634120000000000LLU);
}
END_TEST

START_TEST (test_send_with_null_writer)
{
    openxc_DynamicField field;
    field.has_numeric_value = true;
    field.numeric_value = 0xa;

    fail_unless(can::write::sendSignal(&getSignals()[6], &field,
                NULL, getSignals(), getSignalCount()));
    CanMessage queuedMessage = QUEUE_POP(CanMessage,
            &getSignals()[6].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data, 0x1e);
}
END_TEST

START_TEST (test_send_using_default)
{
    openxc_DynamicField field;
    field.has_numeric_value = true;
    field.numeric_value = 0xa;

    fail_unless(can::write::sendSignal(&getSignals()[6], &field,
                getSignals(), getSignalCount()));
    CanMessage queuedMessage = QUEUE_POP(CanMessage,
            &getSignals()[6].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data, 0x1e);
}
END_TEST

START_TEST (test_send_with_custom_with_states)
{
    openxc_DynamicField field;
    field.has_string_value = true;
    strcpy(field.string_value, getSignals()[1].states[1].name);

    fail_unless(can::write::sendSignal(&getSignals()[1], &field, getSignals(),
                getSignalCount()));
    CanMessage queuedMessage = QUEUE_POP(CanMessage,
            &getSignals()[1].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data, 0x20);
}
END_TEST

float customStateWriter(CanSignal* signal, CanSignal* signals,
        int signalCount, openxc_DynamicField* value, bool* send) {
    *send = false;
    return 0;
}

START_TEST (test_send_with_custom_says_no_send)
{
    openxc_DynamicField field;
    field.has_string_value = true;
    strcpy(field.string_value, getSignals()[1].states[1].name);

    fail_if(can::write::sendSignal(&getSignals()[1],
                &field, customStateWriter, getSignals(), getSignalCount()));
    fail_unless(QUEUE_EMPTY(CanMessage, &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_force_send)
{
    openxc_DynamicField field;
    field.has_string_value = true;
    strcpy(field.string_value, getSignals()[1].states[1].name);

    fail_unless(can::write::sendSignal(&getSignals()[1],
                &field, customStateWriter,
                getSignals(), getSignalCount(), true));
    ck_assert_int_eq(1, QUEUE_LENGTH(CanMessage,
                &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_write_empty)
{
    can::write::processWriteQueue(&getCanBuses()[0]);
}
END_TEST

START_TEST (test_no_write_handler)
{
    openxc_DynamicField field;
    field.has_numeric_value = true;
    field.numeric_value = 0xa;

    can::write::sendSignal(&getSignals()[6], &field, getSignals(),
            getSignalCount());
    getCanBuses()[0].writeHandler = NULL;
    can::write::processWriteQueue(&getCanBuses()[0]);
    fail_unless(QUEUE_EMPTY(CanMessage, &getSignals()[1].message->bus->sendQueue));
}
END_TEST

bool writeHandler(const CanBus* bus, const CanMessage* message) {
    return false;
}

START_TEST (test_failed_write_handler)
{
    openxc_DynamicField field;
    field.has_numeric_value = true;
    field.numeric_value = 0xa;

    can::write::sendSignal(&getSignals()[6], &field, getSignals(),
            getSignalCount());
    getCanBuses()[0].writeHandler = writeHandler;
    can::write::processWriteQueue(&getCanBuses()[0]);
    fail_unless(QUEUE_EMPTY(CanMessage, &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_successful_write)
{
    openxc_DynamicField field;
    field.has_numeric_value = true;
    field.numeric_value = 0xa;

    can::write::sendSignal(&getSignals()[6], &field, getSignals(),
            getSignalCount());
    can::write::processWriteQueue(&getCanBuses()[0]);
    fail_unless(QUEUE_EMPTY(CanMessage, &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_write_multiples)
{
    openxc_DynamicField field;
    field.has_numeric_value = true;
    field.numeric_value = 0xa;

    can::write::sendSignal(&getSignals()[6], &field, getSignals(),
            getSignalCount());
    can::write::sendSignal(&getSignals()[6], &field, getSignals(),
            getSignalCount());
    can::write::processWriteQueue(&getCanBuses()[0]);
    fail_unless(QUEUE_EMPTY(CanMessage, &getSignals()[1].message->bus->sendQueue));
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
