#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "can/canutil.h"
#include "can/canread.h"
#include "can/canwrite.h"
#include "cJSON.h"

namespace usb = openxc::interface::usb;
namespace can = openxc::can;

using openxc::can::read::booleanHandler;
using openxc::can::read::ignoreHandler;
using openxc::can::read::stateHandler;
using openxc::can::read::passthroughHandler;
using openxc::can::read::sendEventedBooleanMessage;
using openxc::can::read::sendEventedStringMessage;
using openxc::can::read::sendEventedFloatMessage;
using openxc::can::read::sendBooleanMessage;
using openxc::can::read::sendNumericalMessage;
using openxc::can::read::sendStringMessage;
using openxc::pipeline::Pipeline;
using openxc::signals::getSignalCount;
using openxc::signals::getSignals;
using openxc::signals::getCanBuses;
using openxc::signals::getMessages;
using openxc::signals::getMessageCount;

const uint64_t BIG_ENDIAN_TEST_DATA = __builtin_bswap64(0xEB00000000000000);

extern Pipeline PIPELINE;
extern UsbDevice USB_DEVICE;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &PIPELINE.usb->endpoints[IN_ENDPOINT_INDEX].queue;

bool queueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

static unsigned long fakeTime = 0;

unsigned long timeMock() {
    return fakeTime;
}

void setup() {
    fakeTime = 0;
    PIPELINE.outputFormat = openxc::pipeline::JSON;
    PIPELINE.usb = &USB_DEVICE;
    usb::initialize(&USB_DEVICE);
    PIPELINE.usb->configured = true;
    for(int i = 0; i < getSignalCount(); i++) {
        getSignals()[i].received = false;
        getSignals()[i].sendSame = true;
        getSignals()[i].frequencyClock = {0};
    }
}

START_TEST (test_passthrough_handler)
{
    bool send = true;
    ck_assert_int_eq(passthroughHandler(&getSignals()[0], getSignals(), getSignalCount(),
                &PIPELINE, 42.0, &send), 42.0);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_handler)
{
    bool send = true;
    fail_unless(booleanHandler(&getSignals()[0], getSignals(), getSignalCount(), &PIPELINE, 1.0, &send));
    fail_unless(send);
    fail_unless(booleanHandler(&getSignals()[0], getSignals(), getSignalCount(), &PIPELINE, 0.5, &send));
    fail_unless(send);
    fail_if(booleanHandler(&getSignals()[0], getSignals(), getSignalCount(), &PIPELINE, 0, &send));
    fail_unless(send);
}
END_TEST

START_TEST (test_ignore_handler)
{
    bool send = true;
    ignoreHandler(&getSignals()[0], getSignals(), getSignalCount(), &PIPELINE, 1.0, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_state_handler)
{
    bool send = true;
    ck_assert_str_eq(stateHandler(&getSignals()[1], getSignals(), getSignalCount(), &PIPELINE, 2, &send),
            getSignals()[1].states[1].name);
    fail_unless(send);
    stateHandler(&getSignals()[1], getSignals(), getSignalCount(), &PIPELINE, 42, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_send_numerical)
{
    fail_unless(queueEmpty());
    sendNumericalMessage("test", 42, &PIPELINE);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"test\",\"value\":42}\r\n");
}
END_TEST

START_TEST (test_preserve_float_precision)
{
    fail_unless(queueEmpty());
    float value = 42.5;
    sendNumericalMessage("test", value, &PIPELINE);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":42.500000}\r\n");
}
END_TEST

START_TEST (test_send_boolean)
{
    fail_unless(queueEmpty());
    sendBooleanMessage("test", false, &PIPELINE);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":false}\r\n");
}
END_TEST

START_TEST (test_send_string)
{
    fail_unless(queueEmpty());
    sendStringMessage("test", "string", &PIPELINE);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":\"string\"}\r\n");
}
END_TEST

START_TEST (test_send_evented_boolean)
{
    fail_unless(queueEmpty());
    sendEventedBooleanMessage("test", "value", false, &PIPELINE);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":\"value\",\"event\":false}\r\n");
}
END_TEST

START_TEST (test_send_evented_string)
{
    fail_unless(queueEmpty());
    sendEventedStringMessage("test", "value", "event", &PIPELINE);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":\"value\",\"event\":\"event\"}\r\n");
}
END_TEST

START_TEST (test_send_evented_float)
{
    fail_unless(queueEmpty());
    sendEventedFloatMessage("test", "value", 43.0, &PIPELINE);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":\"value\",\"event\":43}\r\n");
}
END_TEST

START_TEST (test_passthrough_force_send_changed)
{
    fail_unless(queueEmpty());
    CanMessage message = {getMessages()[2].id, 0x1234};
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &PIPELINE);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &PIPELINE);
    fail_unless(queueEmpty());
    message.data = 0x5678;
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &PIPELINE);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_passthrough_limited_frequency)
{
    getMessages()[1].frequencyClock.timeFunction = timeMock;
    fail_unless(queueEmpty());
    CanMessage message = {getMessages()[1].id, 0x1234};
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &PIPELINE);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &PIPELINE);
    fail_unless(queueEmpty());
    fakeTime += 2000;
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &PIPELINE);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_passthrough_message)
{
    fail_unless(queueEmpty());
    CanMessage message = {42, 0x123456789ABCDEF1LLU, 8};
    can::read::passthroughMessage(&getCanBuses()[0], &message, NULL, 0, &PIPELINE);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"bus\":1,\"id\":42,\"data\":\"0xf1debc9a78563412\"}\r\n");
}
END_TEST

float floatHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* PIPELINE, float value, bool* send) {
    return 42;
}

START_TEST (test_default_handler)
{
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"torque_at_transmission\",\"value\":-19990}\r\n");
}
END_TEST

const char* noSendStringHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* PIPELINE, float value, bool* send) {
    *send = false;
    return NULL;
}

bool noSendBooleanTranslateHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* PIPELINE, float value, bool* send) {
    *send = false;
    return false;
}

START_TEST (test_translate_respects_send_value)
{
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            ignoreHandler, getSignals(), getSignalCount());
    fail_unless(queueEmpty());

    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            noSendStringHandler, getSignals(), getSignalCount());
    fail_unless(queueEmpty());

    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            noSendBooleanTranslateHandler, getSignals(), getSignalCount());
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_translate_float)
{
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            floatHandler, getSignals(), getSignalCount());
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"torque_at_transmission\",\"value\":42}\r\n");
}
END_TEST

int frequencyTestCounter = 0;
float floatHandlerFrequencyTest(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* PIPELINE, float value, bool* send) {
    frequencyTestCounter++;
    return 42;
}

START_TEST (test_translate_float_handler_called_every_time)
{
    frequencyTestCounter = 0;
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            floatHandlerFrequencyTest, getSignals(), getSignalCount());
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            floatHandlerFrequencyTest, getSignals(), getSignalCount());
    ck_assert_int_eq(frequencyTestCounter, 2);
}
END_TEST

bool boolHandlerFrequencyTest(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* PIPELINE, float value, bool* send) {
    frequencyTestCounter++;
    return true;
}

START_TEST (test_translate_bool_handler_called_every_time)
{
    frequencyTestCounter = 0;
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            boolHandlerFrequencyTest, getSignals(), getSignalCount());
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            boolHandlerFrequencyTest, getSignals(), getSignalCount());
    ck_assert_int_eq(frequencyTestCounter, 2);
}
END_TEST

const char* strHandlerFrequencyTest(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* PIPELINE, float value, bool* send) {
    frequencyTestCounter++;
    return "Dude.";
}

START_TEST (test_translate_str_handler_called_every_time)
{
    frequencyTestCounter = 0;
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            strHandlerFrequencyTest, getSignals(), getSignalCount());
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            strHandlerFrequencyTest, getSignals(), getSignalCount());
    ck_assert_int_eq(frequencyTestCounter, 2);
}
END_TEST

const char* stringHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* PIPELINE, float value, bool* send) {
    return "foo";
}

START_TEST (test_translate_string)
{
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            stringHandler, getSignals(), getSignalCount());
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"torque_at_transmission\",\"value\":\"foo\"}\r\n");
}
END_TEST

bool booleanTranslateHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* PIPELINE, float value, bool* send) {
    return false;
}

START_TEST (test_translate_bool)
{
    can::read::translateSignal(&PIPELINE, &getSignals()[2], BIG_ENDIAN_TEST_DATA,
            booleanTranslateHandler, getSignals(), getSignalCount());
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"brake_pedal_status\",\"value\":false}\r\n");
}
END_TEST

START_TEST (test_always_send_first)
{
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_unlimited_frequency)
{
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_limited_frequency)
{
    getSignals()[0].frequencyClock.frequency = 1;
    getSignals()[0].frequencyClock.timeFunction = timeMock;
    fakeTime = 2000;
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_unless(queueEmpty());
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_unless(queueEmpty());
    // mock waiting 1 second
    fakeTime += 1000;
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_if(queueEmpty());
}
END_TEST

float preserveHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* PIPELINE, float value, bool* send) {
    return signal->lastValue;
}

START_TEST (test_preserve_last_value)
{
    can::read::translateSignal(&PIPELINE, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            getSignals(), getSignalCount());
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);

    can::read::translateSignal(&PIPELINE, &getSignals()[0], 0x1234123000000000,
            preserveHandler, getSignals(), getSignalCount());
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"torque_at_transmission\",\"value\":-19990}\r\n");
}
END_TEST

START_TEST (test_dont_send_same)
{
    getSignals()[2].sendSame = false;
    can::read::translateSignal(&PIPELINE, &getSignals()[2], BIG_ENDIAN_TEST_DATA,
            booleanHandler, getSignals(), getSignalCount());
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"brake_pedal_status\",\"value\":true}\r\n");

    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(&PIPELINE, &getSignals()[2], BIG_ENDIAN_TEST_DATA,
            booleanHandler, getSignals(), getSignalCount());
    fail_unless(queueEmpty());
}
END_TEST

Suite* canreadSuite(void) {
    Suite* s = suite_create("canread");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
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
    tcase_add_test(tc_sending, test_send_evented_float);
    tcase_add_test(tc_sending, test_passthrough_message);
    tcase_add_test(tc_sending, test_passthrough_limited_frequency);
    tcase_add_test(tc_sending, test_passthrough_force_send_changed);
    suite_add_tcase(s, tc_sending);

    TCase *tc_translate = tcase_create("translate");
    tcase_add_checked_fixture(tc_translate, setup, NULL);
    tcase_add_test(tc_translate, test_translate_float);
    tcase_add_test(tc_translate, test_translate_string);
    tcase_add_test(tc_translate, test_translate_bool);
    tcase_add_test(tc_translate, test_limited_frequency);
    tcase_add_test(tc_translate, test_unlimited_frequency);
    tcase_add_test(tc_translate, test_always_send_first);
    tcase_add_test(tc_translate, test_preserve_last_value);
    tcase_add_test(tc_translate, test_default_handler);
    tcase_add_test(tc_translate, test_dont_send_same);
    tcase_add_test(tc_translate, test_translate_respects_send_value);
    tcase_add_test(tc_translate,
            test_translate_float_handler_called_every_time);
    tcase_add_test(tc_translate, test_translate_bool_handler_called_every_time);
    tcase_add_test(tc_translate, test_translate_str_handler_called_every_time);
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
