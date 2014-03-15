#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "config.h"
#include "diagnostics.h"
#include "lights.h"
#include "config.h"
#include "pipeline.h"

namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;

using openxc::pipeline::Pipeline;
using openxc::signals::getCanBuses;
using openxc::signals::getActiveMessageSet;
using openxc::commands::handleIncomingMessage;
using openxc::commands::handleControlCommand;
using openxc::commands::validate;
using openxc::commands::Command;
using openxc::config::getConfiguration;
using openxc::config::getFirmwareDescriptor;

extern void initializeVehicleInterface();

extern char LAST_COMMAND_NAME[];
extern openxc_DynamicField LAST_COMMAND_VALUE;
extern openxc_DynamicField LAST_COMMAND_EVENT;
extern uint8_t LAST_CONTROL_COMMAND_PAYLOAD[];
extern size_t LAST_CONTROL_COMMAND_PAYLOAD_LENGTH;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[IN_ENDPOINT_INDEX].queue;

openxc_VehicleMessage RAW_MESSAGE;
openxc_VehicleMessage TRANSLATED_MESSAGE;
openxc_VehicleMessage CONTROL_COMMAND;

bool outputQueueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

static bool canQueueEmpty(int bus) {
    return QUEUE_EMPTY(CanMessage, &getCanBuses()[bus].sendQueue);
}

void setup() {
    initializeVehicleInterface();
    getConfiguration()->usb.configured = true;
    fail_unless(canQueueEmpty(0));
    getActiveMessageSet()->busCount = 2;
    getCanBuses()[0].rawWritable = true;

    RAW_MESSAGE.has_type = true;
    RAW_MESSAGE.type = openxc_VehicleMessage_Type_RAW;
    RAW_MESSAGE.has_raw_message = true;
    RAW_MESSAGE.raw_message.has_message_id = true;
    RAW_MESSAGE.raw_message.message_id = 1;
    RAW_MESSAGE.raw_message.has_data = true;
    RAW_MESSAGE.raw_message.data.bytes[0] = 0xff;
    RAW_MESSAGE.raw_message.has_bus = true;
    RAW_MESSAGE.raw_message.bus = 2;

    TRANSLATED_MESSAGE.has_type = true;
    TRANSLATED_MESSAGE.type = openxc_VehicleMessage_Type_TRANSLATED;
    TRANSLATED_MESSAGE.has_translated_message = true;
    TRANSLATED_MESSAGE.translated_message.has_type = true;
    TRANSLATED_MESSAGE.translated_message.type = openxc_TranslatedMessage_Type_NUM;
    TRANSLATED_MESSAGE.translated_message.has_name = true;
    strcpy(TRANSLATED_MESSAGE.translated_message.name, "foo");
    TRANSLATED_MESSAGE.translated_message.has_value = true;
    TRANSLATED_MESSAGE.translated_message.value.has_type = true;
    TRANSLATED_MESSAGE.translated_message.value.type = openxc_DynamicField_Type_NUM;
    TRANSLATED_MESSAGE.translated_message.value.numeric_value = 42;

    CONTROL_COMMAND.has_type = true;
    CONTROL_COMMAND.type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
    CONTROL_COMMAND.has_control_command = true;
    CONTROL_COMMAND.control_command.has_type = true;
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_DIAGNOSTIC;
    CONTROL_COMMAND.control_command.has_diagnostic_request = true;
    CONTROL_COMMAND.control_command.diagnostic_request.has_bus = true;
    CONTROL_COMMAND.control_command.diagnostic_request.bus = 1;
    CONTROL_COMMAND.control_command.diagnostic_request.has_message_id = true;
    CONTROL_COMMAND.control_command.diagnostic_request.message_id = 2;
    CONTROL_COMMAND.control_command.diagnostic_request.has_mode = true;
    CONTROL_COMMAND.control_command.diagnostic_request.mode = 22;
    CONTROL_COMMAND.control_command.diagnostic_request.has_pid = true;
    CONTROL_COMMAND.control_command.diagnostic_request.pid = 23;
    CONTROL_COMMAND.control_command.diagnostic_request.has_payload = true;
    CONTROL_COMMAND.control_command.diagnostic_request.payload.bytes[0] = 0xff;
    CONTROL_COMMAND.control_command.diagnostic_request.payload.size = 1;
    CONTROL_COMMAND.control_command.diagnostic_request.has_parse_payload = true;
    CONTROL_COMMAND.control_command.diagnostic_request.parse_payload = true;
    CONTROL_COMMAND.control_command.diagnostic_request.has_multiple_responses = true;
    CONTROL_COMMAND.control_command.diagnostic_request.multiple_responses = false;
    CONTROL_COMMAND.control_command.diagnostic_request.has_factor = true;
    CONTROL_COMMAND.control_command.diagnostic_request.factor = 2.1;
    CONTROL_COMMAND.control_command.diagnostic_request.has_offset = true;
    CONTROL_COMMAND.control_command.diagnostic_request.offset = -1000.2;;
    CONTROL_COMMAND.control_command.diagnostic_request.has_frequency = true;
    CONTROL_COMMAND.control_command.diagnostic_request.frequency = 10;
}

const char* RAW_REQUEST = "{\"bus\": 1, \"id\": 42, \"data\": \"0x1234\"}";
const char* WRITABLE_TRANSLATED_REQUEST = "{\"name\": \"transmission_gear_position\", \"value\": \"third\"}";
const char* NON_WRITABLE_TRANSLATED_REQUEST = "{\"name\": \"torque_at_transmission\", \"value\": 200}";
const char* DIAGNOSTIC_REQUEST = "{\"command\": \"diagnostic_request\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1}}";

START_TEST (test_raw_write_no_matching_bus)
{
    getCanBuses()[0].rawWritable = true;
    const char* request = "{\"bus\": 3, \"id\": 42, \"data\": \"0x1234\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    fail_unless(canQueueEmpty(0));
    fail_unless(canQueueEmpty(1));
}
END_TEST

START_TEST (test_raw_write_missing_bus)
{
    getCanBuses()[0].rawWritable = true;
    const char* request = "{\"id\": 42, \"data\": \"0x1234\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write_missing_bus_no_buses)
{
    getCanBuses()[0].rawWritable = true;
    getActiveMessageSet()->busCount = 0;
    const char* request = "{\"id\": 42, \"data\": \"0x1234\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write)
{
    getCanBuses()[0].rawWritable = true;
    const char* request = "{\"bus\": 1, \"id\": 42, \"data\": \"0x1234567812345678\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.id, 42);
    ck_assert_int_eq(message.data, 0x1234567812345678);
}
END_TEST

START_TEST (test_raw_write_less_than_full_message)
{
    getCanBuses()[0].rawWritable = true;
    ck_assert(handleIncomingMessage((uint8_t*)RAW_REQUEST,
                strlen(RAW_REQUEST)));
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.id, 42);
    ck_assert_int_eq(message.data, 0x1234);
    // TODO pending
    // ck_assert_int_eq(message.length, 2);
}
END_TEST

START_TEST (test_raw_write_not_allowed)
{
    getCanBuses()[0].rawWritable = false;
    ck_assert(handleIncomingMessage((uint8_t*)RAW_REQUEST,
                strlen(RAW_REQUEST)));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_with_payload)
{
    const char* request = "{\"command\": \"diagnostic_request\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1, \"payload\": \"0x1234\"}}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.id, 2);
    // TODO we're not handling payload less than 8 bytes correctly
    // ck_assert_int_eq(message.data, 0x0301123400000000LLU);
}
END_TEST

START_TEST (test_diagnostic_request_missing_request)
{
    const char* request = "{\"command\": \"diagnostic_request\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_write_not_allowed)
{
    getCanBuses()[0].rawWritable = false;
    ck_assert(handleIncomingMessage((uint8_t*)DIAGNOSTIC_REQUEST,
                strlen(DIAGNOSTIC_REQUEST)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request)
{
    ck_assert(handleIncomingMessage((uint8_t*)DIAGNOSTIC_REQUEST,
                strlen(DIAGNOSTIC_REQUEST)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_missing_mode)
{
    const char* request = "{\"command\": \"diagnostic_request\", \"request\": {\"bus\": 1, \"id\": 2}}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_missing_arb_id)
{
    const char* request = "{\"command\": \"diagnostic_request\", \"request\": {\"bus\": 1, \"mode\": 1}}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_missing_bus)
{
    const char* request = "{\"command\": \"diagnostic_request\", \"request\": {\"id\": 2, \"mode\": 1}}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_invalid_bus)
{
    const char* request = "{\"command\": \"diagnostic_request\", \"request\": {\"bus\": 3, \"id\": 2, \"mode\": 1}}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_non_complete_message)
{
    const char* request = "{\"name\": \"turn_signal_status\", ";
    ck_assert(!handleIncomingMessage((uint8_t*)request, strlen(request)));
}
END_TEST

START_TEST (test_custom_evented_command)
{
    const char* command = "{\"name\": \"turn_signal_status\", \"value\": true, \"event\": 2}";
    ck_assert(handleIncomingMessage((uint8_t*)command, strlen(command)));
    ck_assert_str_eq(LAST_COMMAND_NAME, "turn_signal_status");
    ck_assert_int_eq(LAST_COMMAND_VALUE.has_type, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.type, openxc_DynamicField_Type_BOOL);
    ck_assert_int_eq(LAST_COMMAND_VALUE.has_boolean_value, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.boolean_value, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.has_numeric_value, false);
    ck_assert_int_eq(LAST_COMMAND_VALUE.has_string_value, false);
    ck_assert_int_eq(LAST_COMMAND_EVENT.has_type, true);
    ck_assert_int_eq(LAST_COMMAND_EVENT.type, openxc_DynamicField_Type_NUM);
    ck_assert_int_eq(LAST_COMMAND_EVENT.has_numeric_value, true);
    ck_assert_int_eq(LAST_COMMAND_EVENT.numeric_value, 2);
    ck_assert_int_eq(LAST_COMMAND_EVENT.has_boolean_value, false);
    ck_assert_int_eq(LAST_COMMAND_EVENT.has_string_value, false);
}
END_TEST

START_TEST (test_custom_command)
{
    const char* command = "{\"name\": \"turn_signal_status\", \"value\": true}";
    ck_assert(handleIncomingMessage((uint8_t*)command, strlen(command)));
    ck_assert_str_eq(LAST_COMMAND_NAME, "turn_signal_status");
    ck_assert_int_eq(LAST_COMMAND_VALUE.has_type, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.type, openxc_DynamicField_Type_BOOL);
    ck_assert_int_eq(LAST_COMMAND_VALUE.has_boolean_value, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.boolean_value, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.has_numeric_value, false);
    ck_assert_int_eq(LAST_COMMAND_VALUE.has_string_value, false);
}
END_TEST

START_TEST (test_translated_write_no_match)
{
    const char* request = "{\"name\": \"foobar\", \"value\": true}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_translated_write_missing_value)
{
    const char* request = "{\"name\": \"turn_signal_status\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_translated_write_allowed)
{
    ck_assert(handleIncomingMessage((uint8_t*)WRITABLE_TRANSLATED_REQUEST,
                strlen(WRITABLE_TRANSLATED_REQUEST)));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_translated_write_not_allowed)
{
    ck_assert(handleIncomingMessage((uint8_t*)NON_WRITABLE_TRANSLATED_REQUEST,
                strlen(WRITABLE_TRANSLATED_REQUEST)));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_translated_write_allowed_by_signal_override)
{
    getCanBuses()[0].rawWritable = false;
    ck_assert(handleIncomingMessage((uint8_t*)WRITABLE_TRANSLATED_REQUEST,
                strlen(WRITABLE_TRANSLATED_REQUEST)));
    fail_if(canQueueEmpty(0));
    ck_assert(handleIncomingMessage((uint8_t*)NON_WRITABLE_TRANSLATED_REQUEST,
                strlen(WRITABLE_TRANSLATED_REQUEST)));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_unrecognized_command_name)
{
    const char* request = "{\"command\": \"foo\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_unrecognized_message)
{
    const char* request = "{\"foo\": 1, \"bar\": 42, \"data\": \"0x1234\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_device_id_message_in_stream)
{
    const char* request = "{\"command\": \"device_id\"}";
    strcpy(getConfiguration()->uart.deviceId, "mydevice");
    ck_assert(outputQueueEmpty());
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    ck_assert(!outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot, getConfiguration()->uart.deviceId) != NULL);
}
END_TEST

START_TEST (test_version_message_in_stream)
{
    const char* request = "{\"command\": \"version\"}";
    ck_assert(outputQueueEmpty());
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    ck_assert(!outputQueueEmpty());

    char firmwareDescriptor[256] = {0};
    getFirmwareDescriptor(firmwareDescriptor, sizeof(firmwareDescriptor));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot, firmwareDescriptor) != NULL);
}
END_TEST

START_TEST (test_version_message)
{
    const char* request = "{\"command\": \"version\"}";
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    char firmwareDescriptor[256] = {0};
    getFirmwareDescriptor(firmwareDescriptor, sizeof(firmwareDescriptor));
    ck_assert(LAST_CONTROL_COMMAND_PAYLOAD_LENGTH > 0);
    ck_assert(strstr((char*)LAST_CONTROL_COMMAND_PAYLOAD, firmwareDescriptor) != NULL);
}
END_TEST

START_TEST (test_version_control_command)
{
    ck_assert(handleControlCommand(Command::VERSION, NULL, 0));
    char firmwareDescriptor[256] = {0};
    getFirmwareDescriptor(firmwareDescriptor, sizeof(firmwareDescriptor));
    ck_assert(LAST_CONTROL_COMMAND_PAYLOAD_LENGTH > 0);
    ck_assert(strstr((char*)LAST_CONTROL_COMMAND_PAYLOAD, firmwareDescriptor) != NULL);
}
END_TEST

START_TEST (test_device_id_message)
{
    const char* request = "{\"command\": \"device_id\"}";
    strcpy(getConfiguration()->uart.deviceId, "mydevice");
    ck_assert(handleIncomingMessage((uint8_t*)request, strlen(request)));
    ck_assert(LAST_CONTROL_COMMAND_PAYLOAD_LENGTH > 0);
    ck_assert(strstr((char*)LAST_CONTROL_COMMAND_PAYLOAD, getConfiguration()->uart.deviceId) != NULL);
}
END_TEST

START_TEST (test_device_id_control_command)
{
    strcpy(getConfiguration()->uart.deviceId, "mydevice");
    ck_assert(handleControlCommand(Command::DEVICE_ID, NULL, 0));
    ck_assert(LAST_CONTROL_COMMAND_PAYLOAD_LENGTH > 0);
    ck_assert(strstr((char*)LAST_CONTROL_COMMAND_PAYLOAD, getConfiguration()->uart.deviceId) != NULL);
}
END_TEST

START_TEST (test_complex_control_command)
{
    ck_assert(handleControlCommand(Command::COMPLEX_COMMAND,
                (uint8_t*)WRITABLE_TRANSLATED_REQUEST,
                strlen(WRITABLE_TRANSLATED_REQUEST)));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_unrecognized_control_command)
{
    ck_assert(!handleControlCommand(Command(1), NULL, 0));
}
END_TEST

START_TEST (test_validate_raw)
{
    ck_assert(validate(&RAW_MESSAGE));
}
END_TEST

START_TEST (test_validate_raw_missing_bus)
{
    RAW_MESSAGE.raw_message.has_bus = false;
    ck_assert(validate(&RAW_MESSAGE));
}
END_TEST

START_TEST (test_validate_missing_type)
{
    RAW_MESSAGE.has_type = false;
    ck_assert(!validate(&RAW_MESSAGE));
}
END_TEST

START_TEST (test_validate_raw_missing_id)
{
    RAW_MESSAGE.raw_message.has_message_id = false;
    ck_assert(!validate(&RAW_MESSAGE));
}
END_TEST

START_TEST (test_validate_raw_missing_data)
{
    RAW_MESSAGE.raw_message.has_data = false;
    ck_assert(!validate(&RAW_MESSAGE));
}
END_TEST

START_TEST (test_validate_translated)
{
    ck_assert(validate(&TRANSLATED_MESSAGE));
}
END_TEST

START_TEST (test_validate_translated_bad_value)
{
    TRANSLATED_MESSAGE.translated_message.value.has_type = false;
    ck_assert(!validate(&TRANSLATED_MESSAGE));
}
END_TEST

START_TEST (test_validate_translated_missing_name)
{
    TRANSLATED_MESSAGE.translated_message.has_name = false;
    ck_assert(!validate(&TRANSLATED_MESSAGE));
}
END_TEST

START_TEST (test_validate_translated_missing_value)
{
    TRANSLATED_MESSAGE.translated_message.has_value = false;
    ck_assert(!validate(&TRANSLATED_MESSAGE));
}
END_TEST

START_TEST (test_validate_translated_bad_event)
{
    TRANSLATED_MESSAGE.translated_message.has_event = true;
    TRANSLATED_MESSAGE.translated_message.event.has_type = false;
    ck_assert(!validate(&TRANSLATED_MESSAGE));
}
END_TEST

START_TEST (test_validate_diagnostic)
{
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_missing_mode)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_mode = false;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_missing_bus)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_bus = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_missing_id)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_message_id = false;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_pid)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_pid = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_payload)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_payload = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_parse_payload)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_parse_payload = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_multiple_responses)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_multiple_responses = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_factor)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_factor = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_offset)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_offset = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_frequency)
{
    CONTROL_COMMAND.control_command.diagnostic_request.has_frequency = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_version_command)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_VERSION;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_device_id_command)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_DEVICE_ID;
    ck_assert(validate(&CONTROL_COMMAND));

    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_DEVICE_ID;
    CONTROL_COMMAND.control_command.has_type = false;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("commands");
    TCase *tc_complex_commands = tcase_create("complex_commands");
    tcase_add_checked_fixture(tc_complex_commands, setup, NULL);
    tcase_add_test(tc_complex_commands, test_non_complete_message);
    tcase_add_test(tc_complex_commands, test_raw_write_no_matching_bus);
    tcase_add_test(tc_complex_commands, test_raw_write_missing_bus);
    tcase_add_test(tc_complex_commands, test_raw_write_missing_bus_no_buses);
    tcase_add_test(tc_complex_commands, test_raw_write);
    tcase_add_test(tc_complex_commands, test_raw_write_less_than_full_message);
    tcase_add_test(tc_complex_commands, test_raw_write_not_allowed);
    tcase_add_test(tc_complex_commands, test_translated_write_allowed);
    tcase_add_test(tc_complex_commands, test_translated_write_not_allowed);
    tcase_add_test(tc_complex_commands, test_translated_write_missing_value);
    tcase_add_test(tc_complex_commands, test_translated_write_no_match);
    tcase_add_test(tc_complex_commands, test_custom_command);
    tcase_add_test(tc_complex_commands, test_custom_evented_command);
    tcase_add_test(tc_complex_commands,
            test_translated_write_allowed_by_signal_override);
    tcase_add_test(tc_complex_commands, test_unrecognized_message);
    tcase_add_test(tc_complex_commands, test_unrecognized_command_name);
    // These tests are mixing up payload deserialization and request handling
    // because they used to be more tightly coupled, so we could clean this up a
    // bit. It's low priority though, since the code is still all tested - the
    // test suite might just get a little smaller.
    tcase_add_test(tc_complex_commands, test_diagnostic_request);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_write_not_allowed);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_with_payload);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_missing_request);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_invalid_bus);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_missing_bus);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_missing_arb_id);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_missing_mode);
    suite_add_tcase(s, tc_complex_commands);

    TCase *tc_control_commands = tcase_create("control_commands");
    tcase_add_checked_fixture(tc_control_commands, setup, NULL);
    tcase_add_test(tc_control_commands, test_version_message);
    tcase_add_test(tc_control_commands, test_version_control_command);
    tcase_add_test(tc_control_commands, test_version_message_in_stream);
    tcase_add_test(tc_control_commands, test_device_id_message);
    tcase_add_test(tc_control_commands, test_device_id_control_command);
    tcase_add_test(tc_control_commands, test_device_id_message_in_stream);
    tcase_add_test(tc_control_commands, test_unrecognized_control_command);
    tcase_add_test(tc_control_commands, test_complex_control_command);
    suite_add_tcase(s, tc_control_commands);

    TCase *tc_validation = tcase_create("validation");
    tcase_add_checked_fixture(tc_validation, setup, NULL);
    tcase_add_test(tc_validation, test_validate_missing_type);
    tcase_add_test(tc_validation, test_validate_raw);
    tcase_add_test(tc_validation, test_validate_raw_missing_bus);
    tcase_add_test(tc_validation, test_validate_raw_missing_id);
    tcase_add_test(tc_validation, test_validate_raw_missing_data);
    tcase_add_test(tc_validation, test_validate_translated);
    tcase_add_test(tc_validation, test_validate_translated_bad_value);
    tcase_add_test(tc_validation, test_validate_translated_missing_name);
    tcase_add_test(tc_validation, test_validate_translated_missing_value);
    tcase_add_test(tc_validation, test_validate_translated_bad_event);
    tcase_add_test(tc_validation, test_validate_diagnostic);
    tcase_add_test(tc_validation, test_validate_diagnostic_missing_mode);
    tcase_add_test(tc_validation, test_validate_diagnostic_missing_bus);
    tcase_add_test(tc_validation, test_validate_diagnostic_missing_id);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_pid);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_payload);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_parse_payload);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_factor);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_offset);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_frequency);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_multiple_responses);
    tcase_add_test(tc_validation, test_validate_version_command);
    tcase_add_test(tc_validation, test_validate_device_id_command);
    suite_add_tcase(s, tc_validation);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = suite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
