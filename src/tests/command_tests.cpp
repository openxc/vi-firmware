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
using openxc::commands::handleIncomingMessage;
using openxc::commands::handleControlCommand;
using openxc::commands::Command;
using openxc::config::getConfiguration;
using openxc::config::getFirmwareDescriptor;

extern void initializeVehicleInterface();

extern char LAST_COMMAND_NAME[];
extern openxc_DynamicField LAST_COMMAND_VALUE;
extern openxc_DynamicField LAST_COMMAND_EVENT;
extern uint8_t LAST_CONTROL_COMMAND_PAYLOAD[];
extern size_t LAST_CONTROL_COMMAND_PAYLOAD_LENGTH;

static bool canQueueEmpty(int bus) {
    return QUEUE_EMPTY(CanMessage, &getCanBuses()[bus].sendQueue);
}

void setup() {
    initializeVehicleInterface();
    fail_unless(canQueueEmpty(0));
}

const char* RAW_REQUEST = "{\"bus\": 1, \"id\": 42, \"data\": \"0x1234\"}";
const char* WRITABLE_TRANSLATED_REQUEST = "{\"name\": \"transmission_gear_position\", \"value\": \"third\"}";
const char* NON_WRITABLE_TRANSLATED_REQUEST = "{\"name\": \"torque_at_transmission\", \"value\": 200}";
const char* DIAGNOSTIC_REQUEST = "{\"command\": \"diagnostic_request\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1}}";

START_TEST (test_raw_write_allowed)
{
    getCanBuses()[0].rawWritable = true;
    ck_assert(handleIncomingMessage((uint8_t*)RAW_REQUEST,
                strlen(RAW_REQUEST)));
    fail_if(canQueueEmpty(0));
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

START_TEST (test_diagnostic_request)
{
    ck_assert(handleIncomingMessage((uint8_t*)DIAGNOSTIC_REQUEST,
                strlen(DIAGNOSTIC_REQUEST)));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
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

Suite* suite(void) {
    Suite* s = suite_create("commands");
    TCase *tc_complex_commands = tcase_create("complex_commands");
    tcase_add_checked_fixture(tc_complex_commands, setup, NULL);
    tcase_add_test(tc_complex_commands, test_non_complete_message);
    tcase_add_test(tc_complex_commands, test_raw_write_allowed);
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
    tcase_add_test(tc_complex_commands, test_diagnostic_request);
    suite_add_tcase(s, tc_complex_commands);

    TCase *tc_control_commands = tcase_create("control_commands");
    tcase_add_checked_fixture(tc_control_commands, setup, NULL);
    tcase_add_test(tc_control_commands, test_version_message);
    tcase_add_test(tc_control_commands, test_version_control_command);
    tcase_add_test(tc_control_commands, test_device_id_message);
    tcase_add_test(tc_control_commands, test_device_id_control_command);
    tcase_add_test(tc_control_commands, test_unrecognized_control_command);
    tcase_add_test(tc_control_commands, test_complex_control_command);
    suite_add_tcase(s, tc_control_commands);


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
