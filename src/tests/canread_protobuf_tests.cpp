#include <check.h>
#include <stdint.h>
#include "util/log.h"
#include <cJSON.h>
#include <pb_decode.h>

#include "signals.h"
#include "config.h"
#include "can/canutil.h"
#include "can/canread.h"
#include "can/canwrite.h"

namespace usb = openxc::interface::usb;
namespace can = openxc::can;

using openxc::util::log::debug;
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
using openxc::config::getConfiguration;

const uint64_t BIG_ENDIAN_TEST_DATA = __builtin_bswap64(0xEB00000000000000);

extern void initializeVehicleInterface();

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[IN_ENDPOINT_INDEX].queue;

bool queueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

void setup() {
    getConfiguration()->payloadFormat = openxc::payload::PayloadFormat::PROTOBUF;
    initializeVehicleInterface();
    usb::initialize(&getConfiguration()->usb);
    getConfiguration()->usb.configured = true;
    for(int i = 0; i < getSignalCount(); i++) {
        getSignals()[i].received = false;
        getSignals()[i].sendSame = true;
        getSignals()[i].frequencyClock = {0};
    }
}

openxc_VehicleMessage decodeProtobufMessage(Pipeline* pipeline) {
    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &pipeline->usb->endpoints[IN_ENDPOINT_INDEX].queue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &pipeline->usb->endpoints[IN_ENDPOINT_INDEX].queue, snapshot, sizeof(snapshot));

    openxc_VehicleMessage decodedMessage;
    pb_istream_t stream = pb_istream_from_buffer(snapshot, sizeof(snapshot));
    bool status = pb_decode_delimited(&stream, openxc_VehicleMessage_fields, &decodedMessage);
    ck_assert_msg(status, PB_GET_ERROR(&stream));
    debug("Deserialized: ");
    for(unsigned int i = 0; i < sizeof(snapshot); i++) {
        debug("%02x", snapshot[i]);
    }
    debug("");
    return decodedMessage;
}

START_TEST (test_passthrough_message)
{
    fail_unless(queueEmpty());
    CanMessage message = {42, 0x123456789ABCDEF1LLU, 8};
    can::read::passthroughMessage(&(getCanBuses()[0]), &message, NULL, 0, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_RAW, decodedMessage.type);
    ck_assert_int_eq(message.id, decodedMessage.raw_message.message_id);

    union {
        uint64_t whole;
        uint8_t bytes[8];
    } combined;
    memcpy(combined.bytes, decodedMessage.raw_message.data.bytes,
            decodedMessage.raw_message.data.size);
    ck_assert_int_eq(message.data, combined.whole);
}
END_TEST

START_TEST (test_send_numeric_value)
{
    fail_unless(queueEmpty());
    sendNumericalMessage("test", 42, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.translated_message.name);
    ck_assert_int_eq(42, decodedMessage.translated_message.value.numeric_value);
}
END_TEST

START_TEST (test_preserve_float_precision)
{
    fail_unless(queueEmpty());
    float value = 42.5;
    sendNumericalMessage("test", value, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.translated_message.name);
    ck_assert_int_eq(42.5, decodedMessage.translated_message.value.numeric_value);
}
END_TEST

START_TEST (test_send_boolean)
{
    fail_unless(queueEmpty());
    sendBooleanMessage("test", false, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.translated_message.name);
    ck_assert(decodedMessage.translated_message.value.boolean_value == false);
}
END_TEST

START_TEST (test_send_string)
{
    fail_unless(queueEmpty());
    sendStringMessage("test", "string", &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.translated_message.name);
    ck_assert_str_eq("string", decodedMessage.translated_message.value.string_value);
}
END_TEST

// TODO we can't handle evented measurements...would be too many sub-types

const char* stringHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    return "foo";
}

START_TEST (test_translate_string)
{
    can::read::translateSignal(&getConfiguration()->pipeline, &getSignals()[1], BIG_ENDIAN_TEST_DATA,
            stringHandler, getSignals(), getSignalCount());
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("transmission_gear_position", decodedMessage.translated_message.name);
    ck_assert_str_eq("foo", decodedMessage.translated_message.value.string_value);
}
END_TEST

bool booleanTranslateHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    return false;
}

START_TEST (test_translate_bool)
{
    can::read::translateSignal(&getConfiguration()->pipeline, &getSignals()[2], BIG_ENDIAN_TEST_DATA,
            booleanTranslateHandler, getSignals(), getSignalCount());
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("brake_pedal_status", decodedMessage.translated_message.name);
    ck_assert(decodedMessage.translated_message.value.boolean_value == false);
}
END_TEST

float floatHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    return 42;
}

START_TEST (test_translate_float)
{
    can::read::translateSignal(&getConfiguration()->pipeline, &getSignals()[0], BIG_ENDIAN_TEST_DATA,
            floatHandler, getSignals(), getSignalCount());
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_TRANSLATED, decodedMessage.type);
    ck_assert_str_eq("torque_at_transmission", decodedMessage.translated_message.name);
    ck_assert_int_eq(42, decodedMessage.translated_message.value.numeric_value);
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
