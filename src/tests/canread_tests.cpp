#include <check.h>
#include <stdint.h>
#include <string>
#include "signals.h"
#include "can/canutil.h"
#include "can/canread.h"
#include "can/canwrite.h"
#include "pipeline.h"
#include "config.h"

namespace usb = openxc::interface::usb;
namespace can = openxc::can;

using openxc::can::read::booleanDecoder;
using openxc::can::read::ignoreDecoder;
using openxc::can::read::stateDecoder;
using openxc::can::read::noopDecoder;
using openxc::can::read::publishBooleanMessage;
using openxc::can::read::publishNumericalMessage;
using openxc::can::read::publishStringMessage;
using openxc::can::read::publishVehicleMessage;
using openxc::pipeline::Pipeline;
using openxc::signals::getSignalCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalManagers;
using openxc::can::lookupSignalManagerDetails;
using openxc::signals::getCanBuses;
using openxc::signals::getMessages;
using openxc::signals::getMessageCount;
using openxc::config::getConfiguration;

extern bool USB_PROCESSED;
extern size_t SENT_BYTES;

const CanMessage TEST_MESSAGE = {
    id: 0,
    format: STANDARD,
    data: {0xeb},
};

extern unsigned long FAKE_TIME;
extern void initializeVehicleInterface();

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[
        IN_ENDPOINT_INDEX].queue;

bool queueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}


void setup() {
    FAKE_TIME = 1000;
    USB_PROCESSED = false;
    SENT_BYTES = 0;
    initializeVehicleInterface();
    getConfiguration()->payloadFormat = openxc::payload::PayloadFormat::JSON;
    usb::initialize(&getConfiguration()->usb);
    getConfiguration()->usb.configured = true;
    for(int i = 0; i < getSignalCount(); i++) {
        const CanSignal* testSignal = &getSignals()[0];
        SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
        getSignalManagers()[i].received = false;
        ((CanSignal*)testSignal)->sendSame = true;
        signalManager->frequencyClock = {0};
        ((CanSignal*)testSignal)->decoder = NULL;
    }
}

START_TEST (test_passthrough_decoder)
{
    bool send = true;
    
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    openxc_DynamicField decoded = noopDecoder(testSignal, getSignals(), signalManager, 
            getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline, 42.0, &send);
    ck_assert_int_eq(decoded.numeric_value, 42.0);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_decoder)
{
    bool send = true;
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());

    openxc_DynamicField decoded = booleanDecoder(testSignal, getSignals(), signalManager,
    getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline, 1.0, &send);
    ck_assert(decoded.boolean_value);
    fail_unless(send);

    decoded = booleanDecoder(testSignal, getSignals(), signalManager,
    getSignalManagers(), getSignalCount(),&getConfiguration()->pipeline, 0.5, &send);
    ck_assert(decoded.boolean_value);
    fail_unless(send);

    decoded = booleanDecoder(testSignal, getSignals(), signalManager,
    getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline, 0, &send);
    ck_assert(!decoded.boolean_value);
    fail_unless(send);
}
END_TEST

START_TEST (test_ignore_decoder)
{
    bool send = true;
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    ignoreDecoder(testSignal, getSignals(), signalManager,
    getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline, 1.0, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_state_decoder)
{
    bool send = true;
    const CanSignal* testSignal = &getSignals()[1];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    openxc_DynamicField decoded = stateDecoder(testSignal, getSignals(), signalManager,
                getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline, 2, &send);
    ck_assert_str_eq(decoded.string_value, testSignal->states[1].name);
    fail_unless(send);
    stateDecoder(testSignal, getSignals(), signalManager, getSignalManagers(), getSignalCount(),
            &getConfiguration()->pipeline, 42, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_send_numerical)
{
    fail_unless(queueEmpty());
    publishNumericalMessage("test", 42, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"timestamp\":0,\"name\":\"test\",\"value\":42}\0");
}
END_TEST

START_TEST (test_preserve_float_precision)
{
    fail_unless(queueEmpty());
    float value = 42.5;
    publishNumericalMessage("test", value, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"test\",\"value\":42.500000}\0");
}
END_TEST

START_TEST (test_send_boolean)
{
    fail_unless(queueEmpty());
    publishBooleanMessage("test", false, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"test\",\"value\":false}\0");
}
END_TEST

START_TEST (test_send_string)
{
    fail_unless(queueEmpty());
    publishStringMessage("test", "string", &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"test\",\"value\":\"string\"}\0");
}
END_TEST

START_TEST (test_send_evented_boolean)
{
    fail_unless(queueEmpty());

    openxc_DynamicField value = openxc_DynamicField();	// Zero fill
    value.type = openxc_DynamicField_Type_STRING;
    strcpy(value.string_value, "value");

    openxc_DynamicField event = openxc_DynamicField();	// Zero fill
    event.type = openxc_DynamicField_Type_BOOL;
    event.boolean_value = false;

    publishVehicleMessage("test", &value, &event, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"test\",\"value\":\"value\",\"event\":false}\0");
}
END_TEST

START_TEST (test_send_evented_string)
{
    fail_unless(queueEmpty());

    openxc_DynamicField value = openxc_DynamicField();		// Zero fill
    value.type = openxc_DynamicField_Type_STRING;
    strcpy(value.string_value, "value");

    openxc_DynamicField event = openxc_DynamicField();		// Zero fill
    event.type = openxc_DynamicField_Type_STRING;
    strcpy(event.string_value, "event");

    publishVehicleMessage("test", &value, &event, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"test\",\"value\":\"value\",\"event\":\"event\"}\0");
}
END_TEST

START_TEST (test_send_evented_float)
{
    fail_unless(queueEmpty());
    openxc_DynamicField value = openxc_DynamicField();		// Zero fill
    value.type = openxc_DynamicField_Type_STRING;
    strcpy(value.string_value, "value");

    openxc_DynamicField event = openxc_DynamicField();		// Zero fill
    event.type = openxc_DynamicField_Type_NUM;
    event.numeric_value = 43.0;

    publishVehicleMessage("test", &value, &event, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"test\",\"value\":\"value\",\"event\":43}\0");
}
END_TEST

START_TEST (test_passthrough_force_send_changed)
{
    fail_unless(queueEmpty());
    CanMessage message = {
        id: getMessages()[2].id,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34}
    };
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    message.data[0] = 0x56;
    message.data[1] = 0x78;
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_passthrough_limited_frequency)
{
    fail_unless(queueEmpty());
    CanMessage message = {
        id: getMessages()[1].id,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34}
    };
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    FAKE_TIME += 2000;
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_passthrough_message)
{
    fail_unless(queueEmpty());
    CanMessage message = {
        id: 42,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF1},
        length: 8
    };
    can::read::passthroughMessage(&getCanBuses()[0], &message, NULL, 0,
            &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"bus\":1,\"id\":42,\"data\":\"0x123456789abcdef1\"}\0");
}
END_TEST

openxc_DynamicField floatDecoder(const CanSignal* signal,const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField decodedValue = openxc_DynamicField();		// Zero fill
    decodedValue.type = openxc_DynamicField_Type_NUM;
    decodedValue.numeric_value = 42;
    return decodedValue;
}

START_TEST (test_translate_ignore_decoder_still_received)
{
    const CanSignal* testSignal = &getSignals()[0];
    ((CanSignal*)testSignal)->decoder = ignoreDecoder;
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    fail_if(signalManager->received);
    can::read::translateSignal(testSignal, (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    fail_unless(signalManager->received);
}
END_TEST

START_TEST (test_default_decoder)
{
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    can::read::translateSignal(testSignal, (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    fail_unless(signalManager->received);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"torque_at_transmission\",\"value\":-19990}\0");
}
END_TEST

START_TEST (test_translate_respects_send_value)
{
    const CanSignal* testSignal = &getSignals()[0];
    ((CanSignal*)testSignal)->decoder = ignoreDecoder;
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    can::read::translateSignal(testSignal, (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    fail_unless(signalManager->received);
}
END_TEST

START_TEST (test_translate_many_signals)
{
    getConfiguration()->pipeline.uart = NULL;
    ck_assert_int_eq(0, SENT_BYTES);
    for(int i = 7; i < 23; i++) {
        const CanSignal* testSignal = &getSignals()[i];
        SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
        can::read::translateSignal(testSignal,
                (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
        fail_unless(signalManager->received);
    }
    fail_unless(USB_PROCESSED);
    // 8 signals sent - depends on queue size
    //ck_assert_int_eq(11 * 34 + 2, SENT_BYTES);	// Protobuff 2 result
    ck_assert_int_eq(676, SENT_BYTES);
    // 1 in the output queue
    fail_if(queueEmpty());
    //ck_assert_int_eq(1 * 34, QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE));	// Protobuff 2 result
    ck_assert_int_eq(96, QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE));
}
END_TEST

START_TEST (test_translate_float)
{
    const CanSignal* testSignal = &getSignals()[0];
    ((CanSignal*)testSignal)->decoder = &floatDecoder;
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    fail_unless(signalManager->received);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"torque_at_transmission\",\"value\":42}\0");
}
END_TEST

int frequencyTestCounter = 0;
openxc_DynamicField floatDecoderFrequencyTest(const CanSignal* signal,const  CanSignal* signals, SignalManager* signalManager,
 SignalManager* signalManagers, int signalCount, Pipeline* pipeline, float value, bool* send) {
    frequencyTestCounter++;
    return floatDecoder(signal, signals, signalManager, signalManagers, signalCount, pipeline, value, send);
}

START_TEST (test_decoder_called_every_time_with_nonzero_frequency)
{
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    frequencyTestCounter = 0;
    signalManager->frequencyClock.frequency = 1;
    ((CanSignal*)testSignal)->decoder = &floatDecoderFrequencyTest;
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    ck_assert_int_eq(frequencyTestCounter, 2);
}
END_TEST

START_TEST (test_decoder_called_every_time_with_unlimited_frequency)
{
    const CanSignal* testSignal = &getSignals()[0];
    frequencyTestCounter = 0;
    ((CanSignal*)testSignal)->decoder = &floatDecoderFrequencyTest;
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    ck_assert_int_eq(frequencyTestCounter, 2);
}
END_TEST

openxc_DynamicField stringDecoder(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField decodedValue = openxc_DynamicField();		// Zero fill
    decodedValue.type = openxc_DynamicField_Type_STRING;
    strcpy(decodedValue.string_value, "foo");
    return decodedValue;
}

START_TEST (test_translate_string)
{
    const CanSignal* testSignal = &getSignals()[0];
    ((CanSignal*)testSignal)->decoder = stringDecoder;
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"torque_at_transmission\",\"value\":\"foo\"}\0");
}
END_TEST

START_TEST (test_always_send_first)
{
    const CanSignal* testSignal = &getSignals()[0];
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_unlimited_frequency)
{
    const CanSignal* testSignal = &getSignals()[0];
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_limited_frequency)
{
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    signalManager->frequencyClock.frequency = 1;
    FAKE_TIME = 2000;
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    // mock waiting 1 second
    FAKE_TIME += 1000;
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

openxc_DynamicField preserveDecoder(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField decodedValue = openxc_DynamicField();		// Zero fill
    decodedValue.type = openxc_DynamicField_Type_NUM;
    decodedValue.numeric_value = signalManager->lastValue;
    return decodedValue;
}

START_TEST (test_preserve_last_value)
{
    const CanSignal* testSignal = &getSignals()[0];
    can::read::translateSignal(testSignal, (CanMessage*)&TEST_MESSAGE, getSignals(), 
        getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);

    CanMessage message = {
        id: 0,
        format: STANDARD,
        data: {0x12, 0x34, 0x12, 0x30},
    };
    ((CanSignal*)testSignal)->decoder = preserveDecoder;
    can::read::translateSignal(testSignal, (CanMessage*)&message, getSignals(), getSignalManagers(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"torque_at_transmission\",\"value\":-19990}\0");
}
END_TEST

START_TEST (test_dont_send_same)
{
    const CanSignal* testSignal = &getSignals()[2];
    ((CanSignal*)testSignal)->sendSame = false;
    ((CanSignal*)testSignal)->decoder = booleanDecoder;
    can::read::translateSignal(testSignal, (CanMessage*)&TEST_MESSAGE, getSignals(),
            getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"timestamp\":0,\"name\":\"brake_pedal_status\",\"value\":true}\0");

    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(testSignal,
            (CanMessage*)&TEST_MESSAGE, getSignals(), getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

Suite* canreadSuite(void) {
    Suite* s = suite_create("canread");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_passthrough_decoder);
    tcase_add_test(tc_core, test_boolean_decoder);
    tcase_add_test(tc_core, test_ignore_decoder);
    tcase_add_test(tc_core, test_state_decoder);
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
    tcase_add_test(tc_translate, test_limited_frequency);
    tcase_add_test(tc_translate, test_unlimited_frequency);
    tcase_add_test(tc_translate, test_always_send_first);
    tcase_add_test(tc_translate, test_preserve_last_value);
    tcase_add_test(tc_translate, test_translate_ignore_decoder_still_received);
    tcase_add_test(tc_translate, test_default_decoder);
    tcase_add_test(tc_translate, test_dont_send_same);
    tcase_add_test(tc_translate, test_translate_respects_send_value);
    tcase_add_test(tc_translate,
            test_decoder_called_every_time_with_nonzero_frequency);
    tcase_add_test(tc_translate,
            test_decoder_called_every_time_with_unlimited_frequency);
    tcase_add_test(tc_translate,
            test_translate_many_signals);
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
