#include <check.h>
#include <stdint.h>
#include "util/log.h"
#include <pb_decode.h>

#include "signals.h"
#include "config.h"
#include "can/canutil.h"
#include "can/canread.h"
#include "can/canwrite.h"

namespace usb = openxc::interface::usb;
namespace can = openxc::can;

using openxc::util::log::debug;
using openxc::can::read::booleanDecoder;
using openxc::can::read::ignoreDecoder;
using openxc::can::read::stateDecoder;
using openxc::can::read::noopDecoder;
using openxc::can::read::publishBooleanMessage;
using openxc::can::read::publishNumericalMessage;
using openxc::can::read::publishStringMessage;
using openxc::can::lookupSignalManagerDetails;
using openxc::pipeline::Pipeline;
using openxc::signals::getSignalCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalManagers;
using openxc::signals::getCanBuses;
using openxc::config::getConfiguration;

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
        const CanSignal* testSignal = &getSignals()[i];
        SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
        signalManager->received = false;
        ((CanSignal*)testSignal)->sendSame = true;
        signalManager->frequencyClock = {0};
    }
}

openxc_VehicleMessage decodeProtobufMessage(Pipeline* pipeline) {
    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &pipeline->usb->endpoints[IN_ENDPOINT_INDEX].queue) + 1];
    QUEUE_SNAPSHOT(uint8_t, &pipeline->usb->endpoints[IN_ENDPOINT_INDEX].queue, snapshot, sizeof(snapshot));

    openxc_VehicleMessage decodedMessage = openxc_VehicleMessage();	// Zero fill
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
    CanMessage message = {
        id: 42,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF1}
    };
    can::read::passthroughMessage(&(getCanBuses()[0]), &message, NULL, 0,
            &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(
            &getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_CAN, decodedMessage.type);
    ck_assert_int_eq(message.id, decodedMessage.can_message.id);
    for(int i = 0; i < 8; i++) {
        ck_assert_int_eq(message.data[i], decodedMessage.can_message.data.bytes[i]);
    }
}
END_TEST

START_TEST (test_send_numeric_value)
{
    fail_unless(queueEmpty());
    publishNumericalMessage("test", 42, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_SIMPLE, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.simple_message.name);
    ck_assert_int_eq(42, decodedMessage.simple_message.value.numeric_value);
}
END_TEST

START_TEST (test_preserve_float_precision)
{
    fail_unless(queueEmpty());
    float value = 42.5;
    publishNumericalMessage("test", value, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_SIMPLE, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.simple_message.name);
    ck_assert_int_eq(42.5, decodedMessage.simple_message.value.numeric_value);
}
END_TEST

START_TEST (test_send_boolean)
{
    fail_unless(queueEmpty());
    publishBooleanMessage("test", false, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_SIMPLE, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.simple_message.name);
    ck_assert(decodedMessage.simple_message.value.boolean_value == false);
}
END_TEST

START_TEST (test_send_string)
{
    fail_unless(queueEmpty());
    publishStringMessage("test", "string", &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_SIMPLE, decodedMessage.type);
    ck_assert_str_eq("test", decodedMessage.simple_message.name);
    ck_assert_str_eq("string", decodedMessage.simple_message.value.string_value);
}
END_TEST

START_TEST (test_translate_string)
{
    publishStringMessage("transmission_gear_position", "foo", &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_SIMPLE, decodedMessage.type);
    ck_assert_str_eq("transmission_gear_position", decodedMessage.simple_message.name);
    ck_assert_str_eq("foo", decodedMessage.simple_message.value.string_value);
}
END_TEST

START_TEST (test_translate_bool)
{
    publishBooleanMessage("brake_pedal_status", false, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_SIMPLE, decodedMessage.type);
    ck_assert_str_eq("brake_pedal_status", decodedMessage.simple_message.name);
    ck_assert(decodedMessage.simple_message.value.boolean_value == false);
}
END_TEST

START_TEST (test_translate_float)
{
    publishNumericalMessage("torque_at_transmission", 42, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    openxc_VehicleMessage decodedMessage = decodeProtobufMessage(&getConfiguration()->pipeline);
    ck_assert_int_eq(openxc_VehicleMessage_Type_SIMPLE, decodedMessage.type);
    ck_assert_str_eq("torque_at_transmission", decodedMessage.simple_message.name);
    ck_assert_int_eq(42, decodedMessage.simple_message.value.numeric_value);
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
