#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "can/canutil.h"
#include "can/canwrite.h"

namespace can = openxc::can;

using openxc::can::write::encodeDynamicField;
using openxc::can::write::encodeBoolean;
using openxc::can::write::encodeNumber;
using openxc::can::write::encodeState;
using openxc::can::write::buildMessage;
using openxc::signals::getSignalCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalManagers;
using openxc::can::lookupSignalManagerDetails;
using openxc::signals::getCanBuses;

void setup() {
    for(int i = 0; i < getSignalCount(); i++) {
        const CanSignal* testSignal = &getSignals()[i];
        SignalManager* signalManager = openxc::can::lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
        ((CanSignal*)testSignal)->writable = true;
        signalManager->received = false;
        ((CanSignal*)testSignal)->sendSame = true;
        signalManager->frequencyClock = {0};
    }
    QUEUE_INIT(CanMessage, &getCanBuses()[0].sendQueue);
}

START_TEST (test_build_message)
{
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[6], 30, data, sizeof(data));
    ck_assert_int_eq(data[0], 0x1e);
    for(size_t i = 1; i < sizeof(data); i++) {
        ck_assert_int_eq(data[i], 0x0);
    }

    memset(data, 0, sizeof(data));
    buildMessage(&getSignals()[1], 6, data, sizeof(data));
    ck_assert_int_eq(data[0], 0x60);
    for(size_t i = 1; i < sizeof(data); i++) {
        ck_assert_int_eq(data[i], 0x0);
    }

    memset(data, 0, sizeof(data));
    buildMessage(&getSignals()[2], 1, data, sizeof(data));
    ck_assert_int_eq(data[0], 0x80);
    for(size_t i = 1; i < sizeof(data); i++) {
        ck_assert_int_eq(data[i], 0x0);
    }

    memset(data, 0, sizeof(data));
    buildMessage(&getSignals()[2], 0, data, sizeof(data));
    for(size_t i = 0; i < sizeof(data); i++) {
        ck_assert_int_eq(data[i], 0x0);
    }
}
END_TEST

START_TEST (test_encode_invalid_field)
{
    openxc_DynamicField field = openxc_DynamicField();	// Zero Fill

    bool send = true;
    encodeDynamicField(&getSignals()[6], &field, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_encode_number)
{
    bool send = true;
    ck_assert_int_eq(encodeNumber(&getSignals()[6], 0xa, &send), 30);
    fail_unless(send);

    ck_assert_int_eq(encodeNumber(&getSignals()[1], 0x6, &send), 6);
    fail_unless(send);
}
END_TEST

START_TEST (test_encode_boolean)
{
    bool send = true;
    ck_assert_int_eq(encodeBoolean(&getSignals()[2], true, &send), 1);
    fail_unless(send);
    ck_assert_int_eq(encodeBoolean(&getSignals()[2], false, &send), 0);
    fail_unless(send);
}
END_TEST

START_TEST (test_encode_state)
{
    bool send = true;
    ck_assert_int_eq(encodeState(&getSignals()[1],
                getSignals()[1].states[1].name, &send), 2);
    fail_unless(send);
}
END_TEST

START_TEST (test_encode_null_state)
{
    bool send = true;
    encodeState(&getSignals()[1], (const char*)NULL, &send);
    fail_if(send);

    encodeState(&getSignals()[1], NULL, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_encode_unknown_state)
{
    bool send = true;
    encodeState(&getSignals()[1], "not_a_state", &send);
    fail_if(send);
}
END_TEST

START_TEST (test_enqueue_message)
{
    CanMessage message = {
        id: 42,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34, 0x56}
    };
    can::write::enqueueMessage(&getCanBuses()[0], &message);

    ck_assert_int_eq(1, QUEUE_LENGTH(CanMessage, &getCanBuses()[0].sendQueue));
}
END_TEST

START_TEST (test_swaps_byte_order)
{
    CanMessage message = {
        id: 42,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34, 0x56}
    };
    can::write::enqueueMessage(&getCanBuses()[0], &message);

    CanMessage queuedMessage = QUEUE_POP(CanMessage,
            &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(queuedMessage.data[0], 0x12);
    ck_assert_int_eq(queuedMessage.data[1], 0x34);
    ck_assert_int_eq(queuedMessage.data[2], 0x56);
    ck_assert_int_eq(queuedMessage.data[3], 0x0);
}
END_TEST

START_TEST (test_write_not_allowed)
{
    const CanSignal* testSignal = &getSignals()[1];
    ((CanSignal*)testSignal)->writable = false;
    can::write::encodeAndSendNumericSignal(testSignal, 0x6, false);
    fail_unless(QUEUE_EMPTY(CanMessage,
                &testSignal->message->bus->sendQueue));
}
END_TEST

START_TEST (test_send_with_null_writer)
{
    openxc_DynamicField field = openxc_DynamicField();		// Zero Fill
    field.type = openxc_DynamicField_Type_NUM;
    field.numeric_value = 0xa;

    fail_unless(can::write::encodeAndSendSignal(
                &getSignals()[6], &field, NULL, false));
    CanMessage queuedMessage = QUEUE_POP(CanMessage,
            &getSignals()[6].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data[0], 0x1e);
}
END_TEST

START_TEST (test_send_using_default)
{
    openxc_DynamicField field = openxc_DynamicField();		// Zero Fill
    field.type = openxc_DynamicField_Type_NUM;
    field.numeric_value = 0xa;

    fail_unless(can::write::encodeAndSendSignal(
                &getSignals()[6], &field, false));
    CanMessage queuedMessage = QUEUE_POP(CanMessage,
            &getSignals()[6].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data[0], 0x1e);
}
END_TEST

START_TEST (test_send_with_custom_with_states)
{
    fail_unless(can::write::encodeAndSendStateSignal(&getSignals()[1],
                getSignals()[1].states[1].name, false));
    CanMessage queuedMessage = QUEUE_POP(CanMessage,
            &getSignals()[1].message->bus->sendQueue);
    ck_assert_int_eq(queuedMessage.data[0], 0x20);
}
END_TEST

uint64_t customStateWriter(const CanSignal* signal, openxc_DynamicField* value,
        bool* send) {
    *send = false;
    return 0;
}

START_TEST (test_send_with_custom_says_no_send)
{
    openxc_DynamicField field = openxc_DynamicField();	// Zero Fill
    field.type = openxc_DynamicField_Type_STRING;
    strcpy(field.string_value, getSignals()[1].states[1].name);

    fail_if(can::write::encodeAndSendSignal(&getSignals()[1], &field,
                customStateWriter, false));
    fail_unless(QUEUE_EMPTY(CanMessage,
                &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_force_send)
{
    openxc_DynamicField field = openxc_DynamicField();		// Zero Fill
    field.type = openxc_DynamicField_Type_STRING;
    strcpy(field.string_value, getSignals()[1].states[1].name);

    fail_unless(can::write::encodeAndSendSignal(&getSignals()[1],
                &field, customStateWriter, true));
    ck_assert_int_eq(1, QUEUE_LENGTH(CanMessage,
                &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_flush_empty)
{
    can::write::flushOutgoingCanMessageQueue(&getCanBuses()[0]);
}
END_TEST

START_TEST (test_no_flush_handler)
{
    openxc_DynamicField field = openxc_DynamicField();		// Zero Fill
    field.type = openxc_DynamicField_Type_NUM;
    field.numeric_value = 0xa;

    fail_unless(can::write::encodeAndSendSignal(
                &getSignals()[6], &field, false));
    getCanBuses()[0].writeHandler = NULL;
    can::write::flushOutgoingCanMessageQueue(&getCanBuses()[0]);
    fail_unless(QUEUE_EMPTY(CanMessage,
                &getSignals()[6].message->bus->sendQueue));
}
END_TEST

bool writeHandler(CanBus* bus, CanMessage* message) {
    return false;
}

START_TEST (test_failed_flush_handler)
{
    openxc_DynamicField field = openxc_DynamicField();		// Zero Fill
    field.type = openxc_DynamicField_Type_NUM;
    field.numeric_value = 0xa;

    fail_unless(can::write::encodeAndSendSignal(
                &getSignals()[6], &field, false));
    getCanBuses()[0].writeHandler = writeHandler;
    can::write::flushOutgoingCanMessageQueue(&getCanBuses()[0]);
    fail_unless(QUEUE_EMPTY(CanMessage,
                &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_send_numeric)
{
    can::write::encodeAndSendNumericSignal(&getSignals()[6], 0xa, false);
    fail_if(QUEUE_EMPTY(CanMessage, &getCanBuses()[0].sendQueue));
    CanMessage message = QUEUE_POP(CanMessage,
            &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.data[0], 0x1e);
}
END_TEST

START_TEST (test_send_boolean)
{
    can::write::encodeAndSendBooleanSignal(&getSignals()[2], true, false);
    fail_if(QUEUE_EMPTY(CanMessage, &getCanBuses()[0].sendQueue));
    CanMessage message = QUEUE_POP(CanMessage,
            &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.data[0], 0x80);
}
END_TEST

START_TEST (test_send_state)
{
    can::write::encodeAndSendStateSignal(&getSignals()[1],
            getSignals()[1].states[1].name, false);
    fail_if(QUEUE_EMPTY(CanMessage, &getCanBuses()[0].sendQueue));
    CanMessage message = QUEUE_POP(CanMessage,
            &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.data[0], 0x20);
}
END_TEST

START_TEST (test_send_multiples)
{
    can::write::encodeAndSendNumericSignal(&getSignals()[6], 0xa, false);
    can::write::encodeAndSendNumericSignal(&getSignals()[6], 0xa, false);
    fail_if(QUEUE_EMPTY(CanMessage,
                &getSignals()[1].message->bus->sendQueue));
    can::write::flushOutgoingCanMessageQueue(&getCanBuses()[0]);
    fail_unless(QUEUE_EMPTY(CanMessage,
                &getSignals()[1].message->bus->sendQueue));
}
END_TEST

START_TEST (test_basic_flush)
{
    can::write::encodeAndSendNumericSignal(&getSignals()[6], 0xa, false);
    fail_if(QUEUE_EMPTY(CanMessage,
                &getSignals()[1].message->bus->sendQueue));
    can::write::flushOutgoingCanMessageQueue(&getCanBuses()[0]);
    fail_unless(QUEUE_EMPTY(CanMessage,
                &getSignals()[1].message->bus->sendQueue));
}
END_TEST

Suite* canwriteSuite(void) {
    Suite* s = suite_create("canwrite");

    TCase *tc_builders = tcase_create("builders");
    tcase_add_checked_fixture(tc_builders, setup, NULL);
    tcase_add_test(tc_builders, test_build_message);
    suite_add_tcase(s, tc_builders);

    TCase *tc_encoders = tcase_create("encoders");
    tcase_add_checked_fixture(tc_encoders, setup, NULL);
    tcase_add_test(tc_encoders, test_encode_number);
    tcase_add_test(tc_encoders, test_encode_boolean);
    tcase_add_test(tc_encoders, test_encode_state);
    tcase_add_test(tc_encoders, test_encode_null_state);
    tcase_add_test(tc_encoders, test_encode_unknown_state);
    tcase_add_test(tc_encoders, test_encode_invalid_field);
    suite_add_tcase(s, tc_encoders);

    TCase *tc_send = tcase_create("send");
    tcase_add_checked_fixture(tc_send, setup, NULL);
    tcase_add_test(tc_send, test_enqueue_message);
    tcase_add_test(tc_send, test_swaps_byte_order);
    tcase_add_test(tc_send, test_send_using_default);
    tcase_add_test(tc_send, test_send_with_null_writer);
    tcase_add_test(tc_send, test_send_with_custom_with_states);
    tcase_add_test(tc_send, test_send_with_custom_says_no_send);
    tcase_add_test(tc_send, test_write_not_allowed);
    tcase_add_test(tc_send, test_send_numeric);
    tcase_add_test(tc_send, test_send_boolean);
    tcase_add_test(tc_send, test_send_state);
    tcase_add_test(tc_send, test_send_multiples);
    tcase_add_test(tc_send, test_force_send);
    suite_add_tcase(s, tc_send);

    TCase *tc_flush = tcase_create("flush");
    tcase_add_checked_fixture(tc_flush, setup, NULL);
    tcase_add_test(tc_flush, test_flush_empty);
    tcase_add_test(tc_flush, test_basic_flush);
    tcase_add_test(tc_flush, test_no_flush_handler);
    tcase_add_test(tc_flush, test_failed_flush_handler);
    suite_add_tcase(s, tc_flush);

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
