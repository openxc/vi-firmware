#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "config.h"
#include "diagnostics.h"
#include "obd2.h"
#include "lights.h"
#include "config.h"
#include "pipeline.h"

namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;

using openxc::pipeline::Pipeline;
using openxc::signals::getCanBuses;
using openxc::signals::getActiveMessageSet;
using openxc::commands::handleIncomingMessage;
using openxc::commands::validate;
using openxc::config::getConfiguration;
using openxc::config::getFirmwareDescriptor;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::payload::PayloadFormat;
using openxc::interface::InterfaceDescriptor;
using openxc::interface::InterfaceType;

extern void initializeVehicleInterface();

extern char LAST_COMMAND_NAME[];
extern openxc_DynamicField LAST_COMMAND_VALUE;
extern openxc_DynamicField LAST_COMMAND_EVENT;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[
        IN_ENDPOINT_INDEX].queue;

openxc_VehicleMessage CAN_MESSAGE = openxc_VehicleMessage();	// Zero Fill
openxc_VehicleMessage SIMPLE_MESSAGE = openxc_VehicleMessage();	// Zero Fill
openxc_VehicleMessage CONTROL_COMMAND = openxc_VehicleMessage();	// Zero Fill

InterfaceDescriptor DESCRIPTOR = {
    allowRawWrites: true,
    type: InterfaceType::USB
};

bool outputQueueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

static bool canQueueEmpty(int bus) {
    return QUEUE_EMPTY(CanMessage, &getCanBuses()[bus].sendQueue);
}

static void resetQueues() {
    usb::initialize(&getConfiguration()->usb);
    getConfiguration()->usb.configured = true;
    for(int i = 0; i < getCanBusCount(); i++) {
        openxc::can::initializeCommon(&getCanBuses()[i]);
        fail_unless(canQueueEmpty(i));
    }
    fail_unless(outputQueueEmpty());
}

void setup() {
    getConfiguration()->desiredRunLevel = openxc::config::RunLevel::ALL_IO;
    getConfiguration()->obd2BusAddress = 0;
    getConfiguration()->payloadFormat = PayloadFormat::JSON;
    initializeVehicleInterface();
    getConfiguration()->usb.configured = true;
    fail_unless(canQueueEmpty(0));
    getActiveMessageSet()->busCount = 2;
    getCanBuses()[0].rawWritable = true;
    resetQueues();

    //CAN_MESSAGE.has_type = true;
    CAN_MESSAGE.type = openxc_VehicleMessage_Type_CAN;
    //CAN_MESSAGE.has_can_message = true;
    //CAN_MESSAGE.can_message.has_id = true;
    CAN_MESSAGE.can_message.id = 1;
    //CAN_MESSAGE.can_message.has_data = true;
    CAN_MESSAGE.can_message.data.bytes[0] = 0xff;
    //CAN_MESSAGE.can_message.has_bus = true;
    CAN_MESSAGE.can_message.bus = 2;

    //SIMPLE_MESSAGE.has_type = true;
    SIMPLE_MESSAGE.type = openxc_VehicleMessage_Type_SIMPLE;
    //SIMPLE_MESSAGE.has_simple_message = true;
    //SIMPLE_MESSAGE.simple_message.has_name = true;
    strcpy(SIMPLE_MESSAGE.simple_message.name, "foo");
    //SIMPLE_MESSAGE.simple_message.has_value = true;
    //SIMPLE_MESSAGE.simple_message.value.has_type = true;
    SIMPLE_MESSAGE.simple_message.value.type =
            openxc_DynamicField_Type_NUM;
    SIMPLE_MESSAGE.simple_message.value.numeric_value = 42;

    //CONTROL_COMMAND.has_type = true;
    CONTROL_COMMAND.type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
    //CONTROL_COMMAND.has_control_command = true;
    //CONTROL_COMMAND.control_command.has_type = true;
    CONTROL_COMMAND.control_command.type =
            openxc_ControlCommand_Type_DIAGNOSTIC;
    //CONTROL_COMMAND.control_command.has_diagnostic_request = true;
    //CONTROL_COMMAND.control_command.diagnostic_request.has_action = true;
    CONTROL_COMMAND.control_command.diagnostic_request.action =
            openxc_DiagnosticControlCommand_Action_ADD;
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_bus = true;
    CONTROL_COMMAND.control_command.diagnostic_request.request.bus = 1;
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_message_id = true;
    CONTROL_COMMAND.control_command.diagnostic_request.request.message_id = 2;
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_mode = true;
    CONTROL_COMMAND.control_command.diagnostic_request.request.mode = 22;
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_pid = true;
    CONTROL_COMMAND.control_command.diagnostic_request.request.pid = 23;
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_payload = true;
    CONTROL_COMMAND.control_command.diagnostic_request.request.payload.bytes[0] = 0xff;
    CONTROL_COMMAND.control_command.diagnostic_request.request.payload.size = 1;
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_multiple_responses = true;
    CONTROL_COMMAND.control_command.diagnostic_request.request.multiple_responses = false;
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_frequency = true;
    CONTROL_COMMAND.control_command.diagnostic_request.request.frequency = 10;
}

uint8_t CAN_REQUEST[] = "{\"bus\": 1, \"id\": 42, \"data\": \"0x1234\"}\0";
uint8_t WRITABLE_SIMPLE_REQUEST[] = "{\"name\": "
        "\"transmission_gear_position\", \"value\": \"third\"}\0";
uint8_t NON_WRITABLE_SIMPLE_REQUEST[] = "{\"name\":"
        "\"torque_at_transmission\", \"value\": 200}\0";
uint8_t DIAGNOSTIC_REQUEST[] = "{\"command\": \"diagnostic_request\", \"actio"
    "n\": \"add\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1}}\0";

START_TEST (test_raw_write_no_matching_bus)
{
    getCanBuses()[0].rawWritable = true;
    uint8_t request[] = "{\"bus\": 3, \"id\": 42, \"data\": \"0x1234\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_unless(canQueueEmpty(0));
    fail_unless(canQueueEmpty(1));
}
END_TEST

START_TEST (test_raw_write_missing_bus)
{
    getCanBuses()[0].rawWritable = true;
    uint8_t request[] = "{\"id\": 42, \"data\": \"0x1234\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write_missing_bus_no_buses)
{
    getCanBuses()[0].rawWritable = true;
    getActiveMessageSet()->busCount = 0;
    uint8_t request[] = "{\"id\": 42, \"data\": \"0x1234\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write_without_0x_prefix)
{
    getCanBuses()[0].rawWritable = true;
    uint8_t request[] = "{\"bus\": 1, \"id\": 42, \"data\""
            ": \"1234567812345678\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.id, 42);
    ck_assert_int_eq(message.data[0], 0x12);
    ck_assert_int_eq(message.data[1], 0x34);
    ck_assert_int_eq(message.data[2], 0x56);
    ck_assert_int_eq(message.data[3], 0x78);
    ck_assert_int_eq(message.data[4], 0x12);
    ck_assert_int_eq(message.data[5], 0x34);
    ck_assert_int_eq(message.data[6], 0x56);
    ck_assert_int_eq(message.data[7], 0x78);
}
END_TEST

START_TEST (test_raw_write)
{
    //printf("Queue size  (test_raw_write):%d\n", QUEUE_LENGTH(CanMessage, &getCanBuses()[0].sendQueue));
    getCanBuses()[0].rawWritable = true;
    uint8_t request[] = "{\"bus\": 1, \"id\": 42, \"data\": \""
            "0x1234567812345678\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    //printf("message id:%d -- %d -- %d\n", message.id, message.format, message.data[0]);
    ck_assert_int_eq(message.id, 42);
    ck_assert_int_eq(message.format, CanMessageFormat::STANDARD);
    ck_assert_int_eq(message.data[0], 0x12);
    ck_assert_int_eq(message.data[1], 0x34);
    ck_assert_int_eq(message.data[2], 0x56);
    ck_assert_int_eq(message.data[3], 0x78);
    ck_assert_int_eq(message.data[4], 0x12);
    ck_assert_int_eq(message.data[5], 0x34);
    ck_assert_int_eq(message.data[6], 0x56);
    ck_assert_int_eq(message.data[7], 0x78);
}
END_TEST

START_TEST (test_raw_write_with_explicit_format)
{
    getCanBuses()[0].rawWritable = true;
    uint8_t request[] = "{\"bus\": 1, \"id\": 42, \"data\": \""
            "0x1234567812345678\", \"frame_format\": \"extended\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.id, 42);
    ck_assert_int_eq(message.format, CanMessageFormat::EXTENDED);
    ck_assert_int_eq(message.data[0], 0x12);
    ck_assert_int_eq(message.data[1], 0x34);
    ck_assert_int_eq(message.data[2], 0x56);
    ck_assert_int_eq(message.data[3], 0x78);
    ck_assert_int_eq(message.data[4], 0x12);
    ck_assert_int_eq(message.data[5], 0x34);
    ck_assert_int_eq(message.data[6], 0x56);
    ck_assert_int_eq(message.data[7], 0x78);
}
END_TEST

START_TEST (test_raw_write_less_than_full_message)
{
    getCanBuses()[0].rawWritable = true;
    ck_assert(handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST),
                &DESCRIPTOR));
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.id, 42);
    ck_assert_int_eq(message.data[0], 0x12);
    ck_assert_int_eq(message.data[1], 0x34);
    ck_assert_int_eq(message.data[2], 0x0);
    ck_assert_int_eq(message.length, 2);
}
END_TEST

START_TEST (test_raw_write_not_allowed)
{
    getCanBuses()[0].rawWritable = false;
    ck_assert(handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST),
                &DESCRIPTOR));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write_not_allowed_from_source_interface)
{
    getCanBuses()[0].rawWritable = true;
    DESCRIPTOR.allowRawWrites = false;
    DESCRIPTOR.type = InterfaceType::UART;
    ck_assert(handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST),
                &DESCRIPTOR));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write_not_allowed_from_usb)
{
    getCanBuses()[0].rawWritable = true;
    getConfiguration()->usb.descriptor.allowRawWrites = false;
    getConfiguration()->uart.descriptor.allowRawWrites = true;
    getConfiguration()->network.descriptor.allowRawWrites = true;
    ck_assert(openxc::interface::usb::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_unless(canQueueEmpty(0));

    // Make sure we can still write on the other interfaces
    ck_assert(openxc::interface::uart::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_if(canQueueEmpty(0));
    resetQueues();
    ck_assert(openxc::interface::network::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write_not_allowed_from_uart)
{
    getCanBuses()[0].rawWritable = true;
    getConfiguration()->usb.descriptor.allowRawWrites = true;
    getConfiguration()->uart.descriptor.allowRawWrites = false;
    getConfiguration()->network.descriptor.allowRawWrites = true;
    ck_assert(openxc::interface::uart::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_unless(canQueueEmpty(0));

    // Make sure we can still write on the other interfaces
    ck_assert(openxc::interface::usb::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_if(canQueueEmpty(0));
    resetQueues();
    ck_assert(openxc::interface::network::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_raw_write_not_allowed_from_network)
{
    getCanBuses()[0].rawWritable = true;
    getConfiguration()->usb.descriptor.allowRawWrites = true;
    getConfiguration()->uart.descriptor.allowRawWrites = true;
    getConfiguration()->network.descriptor.allowRawWrites = false;
    ck_assert(openxc::interface::network::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_unless(canQueueEmpty(0));

    // Make sure we can still write on the other interfaces
    ck_assert(openxc::interface::usb::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_if(canQueueEmpty(0));
    resetQueues();
    ck_assert(openxc::interface::uart::handleIncomingMessage(CAN_REQUEST, sizeof(CAN_REQUEST)));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_named_diagnostic_request)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\","
           " \"action\": \"add\", \"request\": {\"name\": \"foobar\", "
           "\"bus\": 1, \"id\": 2, \"mode\": 1}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    ck_assert(!LIST_EMPTY(&getConfiguration()
            ->diagnosticsManager.nonrecurringRequests));
    ck_assert_str_eq(LIST_FIRST(&getConfiguration()->diagnosticsManager
            .nonrecurringRequests)->name, "foobar");
}
END_TEST

START_TEST (test_diagnostic_request_with_payload_without_0x_prefix)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": "
            "\"add\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1, "
            "\"payload\": \"1234\"}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.id, 2);
    ck_assert_int_eq(message.data[0], 0x3);
    ck_assert_int_eq(message.data[1], 0x01);
    ck_assert_int_eq(message.data[2], 0x12);
    ck_assert_int_eq(message.data[3], 0x34);
    ck_assert_int_eq(message.data[4], 0x00);
    ck_assert_int_eq(message.data[5], 0x00);
    ck_assert_int_eq(message.data[6], 0x00);
    ck_assert_int_eq(message.data[7], 0x00);
}
END_TEST

START_TEST (test_diagnostic_request_with_payload)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": "
            "\"add\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1, "
            "\"payload\": \"0x1234\"}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.id, 2);
    ck_assert_int_eq(message.data[0], 0x3);
    ck_assert_int_eq(message.data[1], 0x01);
    ck_assert_int_eq(message.data[2], 0x12);
    ck_assert_int_eq(message.data[3], 0x34);
    ck_assert_int_eq(message.data[4], 0x00);
    ck_assert_int_eq(message.data[5], 0x00);
    ck_assert_int_eq(message.data[6], 0x00);
    ck_assert_int_eq(message.data[7], 0x00);
}
END_TEST

START_TEST (test_diagnostic_request_missing_request)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_write_not_allowed)
{
    getCanBuses()[0].rawWritable = false;
    ck_assert(handleIncomingMessage(DIAGNOSTIC_REQUEST,
                sizeof(DIAGNOSTIC_REQUEST), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_explicit_obd2_decoded_type)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": \"add\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1, \"pid\": 4, \"decoded_type\": \"obd2\"}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    ck_assert(!LIST_EMPTY(&getConfiguration()->diagnosticsManager.nonrecurringRequests));
    ck_assert(LIST_FIRST(&getConfiguration()->diagnosticsManager.nonrecurringRequests)->decoder ==
            openxc::diagnostics::obd2::handleObd2Pid);
}
END_TEST

START_TEST (test_recognized_obd2_request_overridden)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": \"add\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1, \"pid\": 4, \"decoded_type\": \"none\"}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    ck_assert(!LIST_EMPTY(&getConfiguration()->diagnosticsManager.nonrecurringRequests));
    ck_assert(LIST_FIRST(&getConfiguration()->diagnosticsManager.nonrecurringRequests)->decoder == openxc::diagnostics::passthroughDecoder);
}
END_TEST

START_TEST (test_recognized_obd2_request)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": \"add\", \"request\": {\"bus\": 1, \"id\": 2, \"mode\": 1, \"pid\": 4}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    ck_assert(!LIST_EMPTY(&getConfiguration()->diagnosticsManager.nonrecurringRequests));
    ck_assert(LIST_FIRST(&getConfiguration()->diagnosticsManager.nonrecurringRequests)->decoder
            == openxc::diagnostics::obd2::handleObd2Pid);
}
END_TEST

START_TEST (test_diagnostic_request)
{
    ck_assert(handleIncomingMessage(DIAGNOSTIC_REQUEST,
                sizeof(DIAGNOSTIC_REQUEST), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
}
END_TEST


START_TEST (test_diagnostic_request_missing_mode)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": \"add\", \"request\": {\"bus\": 1, \"id\": 2}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_missing_arb_id)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": \"add\", \"request\": {\"bus\": 1, \"mode\": 1}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_missing_bus)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": \"add\", \"request\": {\"id\": 2, \"mode\": 1}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_diagnostic_request_invalid_bus)
{
    uint8_t request[] = "{\"command\": \"diagnostic_request\", \"action\": \"add\", \"request\": {\"bus\": 3, \"id\": 2, \"mode\": 1}}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_non_complete_message)
{
    uint8_t request[] = "{\"name\": \"turn_signal_status\", ";
    ck_assert(!handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
}
END_TEST

START_TEST (test_custom_evented_command)
{
    uint8_t command[] = "{\"name\": \"turn_signal_status\", \"value\": true, \"event\": 2}\0";
    ck_assert(handleIncomingMessage(command, sizeof(command), &DESCRIPTOR));
    ck_assert_str_eq(LAST_COMMAND_NAME, "turn_signal_status");
    //ck_assert_int_eq(LAST_COMMAND_VALUE.has_type, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.type, openxc_DynamicField_Type_BOOL);
    //ck_assert_int_eq(LAST_COMMAND_VALUE.has_boolean_value, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.boolean_value, true);
    //ck_assert_int_eq(LAST_COMMAND_VALUE.has_numeric_value, false);
    //ck_assert_int_eq(LAST_COMMAND_VALUE.has_string_value, false);
    //ck_assert_int_eq(LAST_COMMAND_EVENT.has_type, true);
    ck_assert_int_eq(LAST_COMMAND_EVENT.type, openxc_DynamicField_Type_NUM);
    //ck_assert_int_eq(LAST_COMMAND_EVENT.has_numeric_value, true);
    ck_assert_int_eq(LAST_COMMAND_EVENT.numeric_value, 2);
    //ck_assert_int_eq(LAST_COMMAND_EVENT.has_boolean_value, false);
    //ck_assert_int_eq(LAST_COMMAND_EVENT.has_string_value, false);
}
END_TEST

START_TEST (test_custom_command)
{
    uint8_t command[] = "{\"name\": \"turn_signal_status\", \"value\": true}\0";
    ck_assert(handleIncomingMessage(command, sizeof(command), &DESCRIPTOR));
    ck_assert_str_eq(LAST_COMMAND_NAME, "turn_signal_status");
    //ck_assert_int_eq(LAST_COMMAND_VALUE.has_type, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.type, openxc_DynamicField_Type_BOOL);
    //ck_assert_int_eq(LAST_COMMAND_VALUE.has_boolean_value, true);
    ck_assert_int_eq(LAST_COMMAND_VALUE.boolean_value, true);
    //ck_assert_int_eq(LAST_COMMAND_VALUE.has_numeric_value, false);
    //ck_assert_int_eq(LAST_COMMAND_VALUE.has_string_value, false);
}
END_TEST

START_TEST (test_simple_write_no_match)
{
    uint8_t request[] = "{\"name\": \"foobar\", \"value\": true}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_simple_write_missing_value)
{
    uint8_t request[] = "{\"name\": \"turn_signal_status\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_simple_write_allowed)
{
    ck_assert(handleIncomingMessage(WRITABLE_SIMPLE_REQUEST,
                sizeof(WRITABLE_SIMPLE_REQUEST), &DESCRIPTOR));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_simple_write_not_allowed)
{
    ck_assert(handleIncomingMessage(NON_WRITABLE_SIMPLE_REQUEST,
                sizeof(WRITABLE_SIMPLE_REQUEST), &DESCRIPTOR));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_simple_write_allowed_by_signal_override)
{
    getCanBuses()[0].rawWritable = false;
    ck_assert(handleIncomingMessage(WRITABLE_SIMPLE_REQUEST,
                sizeof(WRITABLE_SIMPLE_REQUEST), &DESCRIPTOR));
    fail_if(canQueueEmpty(0));
    ck_assert(handleIncomingMessage(NON_WRITABLE_SIMPLE_REQUEST,
                sizeof(WRITABLE_SIMPLE_REQUEST), &DESCRIPTOR));
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_unrecognized_command_name)
{
    uint8_t request[] = "{\"command\": \"foo\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_unrecognized_message)
{
    uint8_t request[] = "{\"foo\": 1, \"bar\": 42, \"data\": \"0x1234\"}\0";
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_version_message_in_stream)
{
    uint8_t request[] = "{\"command\": \"version\"}\0";
    ck_assert(outputQueueEmpty());
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    ck_assert(!outputQueueEmpty());

    char firmwareDescriptor[256] = {0};
    getFirmwareDescriptor(firmwareDescriptor, sizeof(firmwareDescriptor));

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot, firmwareDescriptor) != NULL);
}
END_TEST

START_TEST (test_device_id_message_in_stream)
{
    uint8_t request[] = "{\"command\": \"device_id\"}\0";
    strcpy(getConfiguration()->uart.deviceId, "mydevice");
    ck_assert(outputQueueEmpty());
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    ck_assert(!outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot,
                getConfiguration()->uart.deviceId) != NULL);
}
END_TEST


START_TEST (test_validate_raw)
{
    ck_assert(validate(&CAN_MESSAGE));
}
END_TEST

START_TEST (test_validate_raw_with_format)
{
    //CAN_MESSAGE.can_message.has_frame_format = true;
    CAN_MESSAGE.can_message.frame_format = openxc_CanMessage_FrameFormat_EXTENDED;
    ck_assert(validate(&CAN_MESSAGE));
}
END_TEST

START_TEST (test_validate_raw_with_format_incompatible_id)
{
    //CAN_MESSAGE.can_message.has_frame_format = true;
    CAN_MESSAGE.can_message.frame_format =
            openxc_CanMessage_FrameFormat_STANDARD;
    CAN_MESSAGE.can_message.id = 0x8ff;
    ck_assert(!validate(&CAN_MESSAGE));
}
END_TEST

START_TEST (test_validate_raw_missing_bus)
{
    //CAN_MESSAGE.can_message.has_bus = false;
    ck_assert(validate(&CAN_MESSAGE));
}
END_TEST

START_TEST (test_validate_missing_type)
{
    //CAN_MESSAGE.has_type = false;
    CAN_MESSAGE.type = openxc_VehicleMessage_Type_UNUSED;
    ck_assert(!validate(&CAN_MESSAGE));
}
END_TEST

START_TEST (test_validate_raw_missing_id)
{
    //CAN_MESSAGE.can_message.has_id = false;
    CAN_MESSAGE.can_message.id = 0;
    ck_assert(!validate(&CAN_MESSAGE));
}
END_TEST

//START_TEST (test_validate_raw_missing_data)
//{
//    //CAN_MESSAGE.can_message.has_data = false;
//    ck_assert(!validate(&CAN_MESSAGE));
//}
//END_TEST

START_TEST (test_validate_simple)
{
    ck_assert(validate(&SIMPLE_MESSAGE));
}
END_TEST

//START_TEST (test_validate_simple_bad_value)
//{
//    //SIMPLE_MESSAGE.simple_message.value.has_type = false;
//    ck_assert(!validate(&SIMPLE_MESSAGE));
//}
//END_TEST

START_TEST (test_validate_simple_missing_name)
{
    //SIMPLE_MESSAGE.simple_message.has_name = false;
    strcpy(SIMPLE_MESSAGE.simple_message.name, "");
    ck_assert(!validate(&SIMPLE_MESSAGE));
}
END_TEST

//START_TEST (test_validate_simple_missing_value)
//{
//    //SIMPLE_MESSAGE.simple_message.has_value = false;
//    ck_assert(!validate(&SIMPLE_MESSAGE));
//}
//END_TEST

//START_TEST (test_validate_simple_bad_event)
//{
//    //SIMPLE_MESSAGE.simple_message.has_event = true;
//    //SIMPLE_MESSAGE.simple_message.event.has_type = false;
//    ck_assert(!validate(&SIMPLE_MESSAGE));
//}
//END_TEST

START_TEST (test_validate_diagnostic)
{
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_missing_action)
{
    //CONTROL_COMMAND.control_command.diagnostic_request.has_action = false;
    CONTROL_COMMAND.control_command.diagnostic_request.action = openxc_DiagnosticControlCommand_Action_UNUSED;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

//START_TEST (test_validate_diagnostic_missing_mode)
//{
//    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_mode = false;
//    ck_assert(!validate(&CONTROL_COMMAND));
//}
//END_TEST

START_TEST (test_validate_diagnostic_missing_bus)
{
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_bus = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_missing_id)
{
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_message_id = false;
    CONTROL_COMMAND.control_command.diagnostic_request.request.message_id = 0;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_pid)
{
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_pid = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_payload)
{
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_payload = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_multiple_responses)
{
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_multiple_responses = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_diagnostic_no_frequency)
{
    //CONTROL_COMMAND.control_command.diagnostic_request.request.has_frequency = false;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_version_command)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_VERSION;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_device_platform_command)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_PLATFORM;
    ck_assert(validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_device_id_command)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_DEVICE_ID;
    ck_assert(validate(&CONTROL_COMMAND));

    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_UNUSED;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_validate_passthrough_commmand)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_PASSTHROUGH;
    //CONTROL_COMMAND.control_command.has_passthrough_mode_request = true;
    //CONTROL_COMMAND.control_command.passthrough_mode_request.has_bus = true;
    CONTROL_COMMAND.control_command.passthrough_mode_request.bus = 1;
    //CONTROL_COMMAND.control_command.passthrough_mode_request.has_enabled = true;
    CONTROL_COMMAND.control_command.passthrough_mode_request.enabled = true;
    ck_assert(validate(&CONTROL_COMMAND));

    //CONTROL_COMMAND.control_command.has_type = false;
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_UNUSED;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_passthrough_request_message)
{
    uint8_t request[] = "{\"command\": \"passthrough\", \"bus\": 1, \"enabled\": true}\0";
    ck_assert(!getCanBuses()[0].passthroughCanMessages);
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    ck_assert(getCanBuses()[0].passthroughCanMessages);
}
END_TEST

START_TEST (test_validate_bypass_command)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS;
    //CONTROL_COMMAND.control_command.has_acceptance_filter_bypass_command = true;
    //CONTROL_COMMAND.control_command.acceptance_filter_bypass_command.has_bus = true;
    CONTROL_COMMAND.control_command.acceptance_filter_bypass_command.bus = 1;
    //CONTROL_COMMAND.control_command.acceptance_filter_bypass_command.has_bypass = true;
    CONTROL_COMMAND.control_command.acceptance_filter_bypass_command.bypass = true;
    ck_assert(validate(&CONTROL_COMMAND));

    //CONTROL_COMMAND.control_command.has_type = false;
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_UNUSED;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_bypass_command)
{
    uint8_t request[] = "{\"command\": \"af_bypass\", \"bus\": 1, \"bypass\": true}\0";
    ck_assert(!getCanBuses()[0].bypassFilters);
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    ck_assert(getCanBuses()[0].bypassFilters);
}
END_TEST

START_TEST (test_validate_payload_format_command)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_PAYLOAD_FORMAT;
    //CONTROL_COMMAND.control_command.has_payload_format_command = true;
    //CONTROL_COMMAND.control_command.payload_format_command.has_format = true;
    CONTROL_COMMAND.control_command.payload_format_command.format =
            openxc_PayloadFormatCommand_PayloadFormat_JSON;
    ck_assert(validate(&CONTROL_COMMAND));

    //CONTROL_COMMAND.control_command.has_type = false;
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_UNUSED;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_payload_format_command)
{
    uint8_t request[] = "{\"command\": \"payload_format\", \"bus\": 1, \"format\": \"protobuf\"}\0";
    ck_assert_int_eq(PayloadFormat::JSON, getConfiguration()->payloadFormat);
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    ck_assert_int_eq(PayloadFormat::PROTOBUF, getConfiguration()->payloadFormat);
}
END_TEST

START_TEST (test_validate_predefined_obd2_command)
{
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS;
    //CONTROL_COMMAND.control_command.has_predefined_obd2_requests_command = true;
    //CONTROL_COMMAND.control_command.predefined_obd2_requests_command.has_enabled = true;
    CONTROL_COMMAND.control_command.predefined_obd2_requests_command.enabled = true;
    ck_assert(validate(&CONTROL_COMMAND));

    //CONTROL_COMMAND.control_command.has_type = false;
    CONTROL_COMMAND.control_command.type = openxc_ControlCommand_Type_UNUSED;
    ck_assert(!validate(&CONTROL_COMMAND));
}
END_TEST

START_TEST (test_predefined_obd2_command)
{
    uint8_t request[] = "{\"command\": \"predefined_obd2\", \"enabled\": true}\0";
    ck_assert(!getConfiguration()->recurringObd2Requests);
    ck_assert(handleIncomingMessage(request, sizeof(request), &DESCRIPTOR));
    ck_assert(getConfiguration()->recurringObd2Requests);
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
    tcase_add_test(tc_complex_commands, test_raw_write_with_explicit_format);
    tcase_add_test(tc_complex_commands, test_raw_write_without_0x_prefix);
    tcase_add_test(tc_complex_commands, test_raw_write_less_than_full_message);
    tcase_add_test(tc_complex_commands, test_raw_write_not_allowed);
    tcase_add_test(tc_complex_commands, test_raw_write_not_allowed_from_source_interface);
    tcase_add_test(tc_complex_commands, test_raw_write_not_allowed_from_usb);
    tcase_add_test(tc_complex_commands, test_raw_write_not_allowed_from_uart);
    tcase_add_test(tc_complex_commands, test_raw_write_not_allowed_from_network);
    tcase_add_test(tc_complex_commands, test_simple_write_allowed);
    tcase_add_test(tc_complex_commands, test_simple_write_not_allowed);
    tcase_add_test(tc_complex_commands, test_simple_write_missing_value);
    tcase_add_test(tc_complex_commands, test_simple_write_no_match);
    tcase_add_test(tc_complex_commands, test_custom_command);
    tcase_add_test(tc_complex_commands, test_custom_evented_command);
    tcase_add_test(tc_complex_commands,
            test_simple_write_allowed_by_signal_override);
    tcase_add_test(tc_complex_commands, test_unrecognized_message);
    tcase_add_test(tc_complex_commands, test_unrecognized_command_name);
    // These tests are mixing up payload deserialization and request handling
    // because they used to be more tightly coupled, so we could clean this up a
    // bit. It's low priority though, since the code is still all tested - the
    // test suite might just get a little smaller.
    tcase_add_test(tc_complex_commands, test_diagnostic_request);
    tcase_add_test(tc_complex_commands,
            test_diagnostic_request_write_not_allowed);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_with_payload);
    tcase_add_test(tc_complex_commands,
            test_diagnostic_request_with_payload_without_0x_prefix);
    tcase_add_test(tc_complex_commands,
            test_diagnostic_request_missing_request);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_invalid_bus);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_missing_bus);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_missing_arb_id);
    tcase_add_test(tc_complex_commands, test_diagnostic_request_missing_mode);
    tcase_add_test(tc_complex_commands, test_named_diagnostic_request);
    tcase_add_test(tc_complex_commands, test_recognized_obd2_request);
    tcase_add_test(tc_complex_commands,
            test_recognized_obd2_request_overridden);
    tcase_add_test(tc_complex_commands, test_explicit_obd2_decoded_type);
    suite_add_tcase(s, tc_complex_commands);

    TCase *tc_control_commands = tcase_create("control_commands");
    tcase_add_checked_fixture(tc_control_commands, setup, NULL);
    tcase_add_test(tc_control_commands, test_version_message_in_stream);
    tcase_add_test(tc_control_commands, test_device_id_message_in_stream);
    tcase_add_test(tc_control_commands, test_passthrough_request_message);
    tcase_add_test(tc_control_commands, test_bypass_command);
    tcase_add_test(tc_control_commands, test_payload_format_command);
    tcase_add_test(tc_control_commands, test_predefined_obd2_command);
    suite_add_tcase(s, tc_control_commands);

    TCase *tc_validation = tcase_create("validation");
    tcase_add_checked_fixture(tc_validation, setup, NULL);
    tcase_add_test(tc_validation, test_validate_missing_type);
    tcase_add_test(tc_validation, test_validate_raw);
    tcase_add_test(tc_validation, test_validate_raw_missing_bus);
    tcase_add_test(tc_validation, test_validate_raw_missing_id);
    //tcase_add_test(tc_validation, test_validate_raw_missing_data);
    tcase_add_test(tc_validation, test_validate_raw_with_format);
    tcase_add_test(tc_validation, test_validate_raw_with_format_incompatible_id);
    tcase_add_test(tc_validation, test_validate_simple);
    //tcase_add_test(tc_validation, test_validate_simple_bad_value);
    tcase_add_test(tc_validation, test_validate_simple_missing_name);
    //tcase_add_test(tc_validation, test_validate_simple_missing_value);
    //tcase_add_test(tc_validation, test_validate_simple_bad_event);
    tcase_add_test(tc_validation, test_validate_diagnostic);
    tcase_add_test(tc_validation, test_validate_diagnostic_missing_action);
    //tcase_add_test(tc_validation, test_validate_diagnostic_missing_mode);
    tcase_add_test(tc_validation, test_validate_diagnostic_missing_bus);
    tcase_add_test(tc_validation, test_validate_diagnostic_missing_id);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_pid);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_payload);
    tcase_add_test(tc_validation, test_validate_diagnostic_no_frequency);
    tcase_add_test(tc_validation,
            test_validate_diagnostic_no_multiple_responses);
    tcase_add_test(tc_validation, test_validate_version_command);
    tcase_add_test(tc_validation, test_validate_device_platform_command);
    tcase_add_test(tc_validation, test_validate_device_id_command);
    tcase_add_test(tc_validation, test_validate_passthrough_commmand);
    tcase_add_test(tc_validation, test_validate_bypass_command);
    tcase_add_test(tc_validation, test_validate_payload_format_command);
    tcase_add_test(tc_validation, test_validate_predefined_obd2_command);
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
