#include <check.h>
#include <stdint.h>
#include "util/log.h"
#include <cJSON.h>
#include <pb_decode.h>

#include "can/canutil.h"
#include "can/canread.h"
#include "can/canwrite.h"

namespace usb = openxc::interface::usb;
namespace can = openxc::can;

using openxc::util::log::debugNoNewline;
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

extern Pipeline PIPELINE;
extern UsbDevice USB_DEVICE;

const uint8_t TEST_DATA[8] = {0xEB};

const int CAN_BUS_COUNT = 2;
CanBus CAN_BUSES[CAN_BUS_COUNT] = {
    { },
    { }
};

const int MESSAGE_COUNT = 3;
CanMessageDefinition MESSAGES[MESSAGE_COUNT] = {
    {&CAN_BUSES[0], 0},
    {&CAN_BUSES[0], 1, {10}},
    {&CAN_BUSES[0], 2, {1}, true},
};

CanSignalState SIGNAL_STATES[1][6] = {
    { {1, "reverse"}, {2, "third"}, {3, "sixth"}, {4, "seventh"},
        {5, "neutral"}, {6, "second"}, },
};

const int SIGNAL_COUNT = 3;
CanSignal SIGNALS[SIGNAL_COUNT] = {
    {&MESSAGES[0], "torque_at_transmission", 2, 4, 1001.0, -30000.000000,
        -5000.000000, 33522.000000, {0}, false, false, NULL, 0, true},
    {&MESSAGES[1], "transmission_gear_position", 1, 3, 1.000000, 0.000000,
        0.000000, 0.000000, {0}, false, false, SIGNAL_STATES[0], 6, true,
        NULL, 4.0},
    {&MESSAGES[2], "brake_pedal_status", 0, 1, 1.000000, 0.000000, 0.000000,
        0.000000, {0}, false, false, NULL, 0, true},
};

const int COMMAND_COUNT = 1;
CanCommand COMMANDS[COMMAND_COUNT] = {
    {"turn_signal_status", NULL},
};

static unsigned long fakeTime = 0;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &PIPELINE.usb->endpoints[IN_ENDPOINT_INDEX].queue;

bool queueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

unsigned long timeMock() {
    return fakeTime;
}

void setup() {
    fakeTime = 0;
    PIPELINE.outputFormat = openxc::pipeline::PROTO;
    PIPELINE.usb = &USB_DEVICE;
    usb::initialize(&USB_DEVICE);
    PIPELINE.usb->configured = true;
    for(int i = 0; i < SIGNAL_COUNT; i++) {
        SIGNALS[i].received = false;
        SIGNALS[i].sendSame = true;
        SIGNALS[i].frequencyClock = {0};
    }
}

openxc_VehicleMessage decodeProtobufMessage(Pipeline* pipeline) {
    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &pipeline->usb->endpoints[IN_ENDPOINT_INDEX].queue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &pipeline->usb->endpoints[IN_ENDPOINT_INDEX].queue, snapshot);

    openxc_VehicleMessage decodedMessage;
    pb_istream_t stream = pb_istream_from_buffer(snapshot, sizeof(snapshot));
    bool status = pb_decode_delimited(&stream, openxc_VehicleMessage_fields, &decodedMessage);
    ck_assert_msg(status, PB_GET_ERROR(&stream));
    debugNoNewline("Deserialized: ");
    for(unsigned int i = 0; i < sizeof(snapshot); i++) {
        debugNoNewline("%02x ", snapshot[i]);
    }
    debug("");
    return decodedMessage;
}

START_TEST (test_passthrough_message)
{
    fail_unless(queueEmpty());
    CanMessage message = {42, {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF1}};
    can::read::passthroughMessage(&CAN_BUSES[0], message.id, message.data,
            NULL, 0, &PIPELINE);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&PIPELINE);
    ck_assert_int_eq(openxc_VehicleMessage_Type_RAW, decodedMessage.type);
    ck_assert_int_eq(message.id, decodedMessage.raw_message.message_id);
    for(int i = 0; i < 8; i++) {
        ck_assert_int_eq(message.data[i], decodedMessage.raw_message.data.bytes[i]);
    }
}
END_TEST

START_TEST (test_send_numeric_value)
{
    fail_unless(queueEmpty());
    sendNumericalMessage("test", 42, &PIPELINE);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&PIPELINE);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.translated_message.name);
    ck_assert_int_eq(42, decodedMessage.translated_message.numeric_value);
}
END_TEST

START_TEST (test_preserve_float_precision)
{
    fail_unless(queueEmpty());
    float value = 42.5;
    sendNumericalMessage("test", value, &PIPELINE);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&PIPELINE);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.translated_message.name);
    ck_assert_int_eq(42.5, decodedMessage.translated_message.numeric_value);
}
END_TEST

START_TEST (test_send_boolean)
{
    fail_unless(queueEmpty());
    sendBooleanMessage("test", false, &PIPELINE);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&PIPELINE);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.translated_message.name);
    ck_assert(decodedMessage.translated_message.boolean_value == false);
}
END_TEST

START_TEST (test_send_string)
{
    fail_unless(queueEmpty());
    sendStringMessage("test", "string", &PIPELINE);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&PIPELINE);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.translated_message.name);
    ck_assert_str_eq("string", decodedMessage.translated_message.string_value);
}
END_TEST

// TODO we can't handle evented measurements...would be too many sub-types

const char* stringHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    return "foo";
}

START_TEST (test_translate_string)
{
    can::read::translateSignal(&PIPELINE, &SIGNALS[1], TEST_DATA,
            stringHandler, SIGNALS, SIGNAL_COUNT);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&PIPELINE);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("transmission_gear_position", decodedMessage.translated_message.name);
    ck_assert_str_eq("foo", decodedMessage.translated_message.string_value);
}
END_TEST

bool booleanTranslateHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    return false;
}

START_TEST (test_translate_bool)
{
    can::read::translateSignal(&PIPELINE, &SIGNALS[2], TEST_DATA,
            booleanTranslateHandler, SIGNALS, SIGNAL_COUNT);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&PIPELINE);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("brake_pedal_status", decodedMessage.translated_message.name);
    ck_assert(decodedMessage.translated_message.boolean_value == false);
}
END_TEST

float floatHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    return 42;
}

START_TEST (test_translate_float)
{
    can::read::translateSignal(&PIPELINE, &SIGNALS[0], TEST_DATA,
            floatHandler, SIGNALS, SIGNAL_COUNT);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&PIPELINE);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("torque_at_transmission", decodedMessage.translated_message.name);
    ck_assert_int_eq(42, decodedMessage.translated_message.numeric_value);
}
END_TEST

Suite* canreadSuite(void) {
    Suite* s = suite_create("canread_protobuf");

    TCase *tc_sending = tcase_create("sending");
    tcase_add_checked_fixture(tc_sending, setup, NULL);
    tcase_add_test(tc_sending, test_send_numeric_value);
    tcase_add_test(tc_sending, test_preserve_float_precision);
    tcase_add_test(tc_sending, test_send_boolean);
    tcase_add_test(tc_sending, test_send_string);
    tcase_add_test(tc_sending, test_passthrough_message);
    suite_add_tcase(s, tc_sending);

    TCase *tc_translate = tcase_create("translate");
    tcase_add_checked_fixture(tc_translate, setup, NULL);
    tcase_add_test(tc_translate, test_translate_float);
    tcase_add_test(tc_translate, test_translate_string);
    tcase_add_test(tc_translate, test_translate_bool);
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
