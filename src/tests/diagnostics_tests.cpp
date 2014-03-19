#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "config.h"
#include "diagnostics.h"

namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;

using openxc::diagnostics::ActiveDiagnosticRequest;
using openxc::diagnostics::DiagnosticsManager;
using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getMessages;
using openxc::pipeline::Pipeline;
using openxc::config::getConfiguration;

extern void initializeVehicleInterface();
extern bool ACCEPTANCE_FILTER_STATUS;
extern long FAKE_TIME;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[IN_ENDPOINT_INDEX].queue;

DiagnosticRequest request = {
    arbitration_id: 0x7e0,
    mode: OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST,
    has_pid: true,
    pid: 0x2,
    pid_length: 1
};

CanMessage message = {
   id: request.arbitration_id + 0x8,
   data: __builtin_bswap64(0x341024500000000),
   length: 8
};

static bool canQueueEmpty(int bus) {
    return QUEUE_EMPTY(CanMessage, &getCanBuses()[bus].sendQueue);
}

bool outputQueueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
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
    ACCEPTANCE_FILTER_STATUS = true;
    getCanBuses()[0].rawWritable = true;
    request.pid = 2;
    request.arbitration_id = 0x7e0;
    initializeVehicleInterface();
    getConfiguration()->payloadFormat = openxc::payload::PayloadFormat::JSON;
    resetQueues();
    diagnostics::initialize(&getConfiguration()->diagnosticsManager, getCanBuses(),
            getCanBusCount(), NULL);
}

START_TEST (test_add_recurring_too_frequent)
{
    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 1));
    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 10));
    ck_assert(!diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 11));
}
END_TEST

START_TEST (test_update_existing_recurring)
{
    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 10));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    // get around the staggered start
    FAKE_TIME += 2000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    // received one response to recurring request - now reset queues

    resetQueues();

    // change request to non-recurring, which should trigger it to be sent once,
    // right now
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    resetQueues();

    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_unless(outputQueueEmpty());
}
END_TEST

START_TEST (test_simultaneous_recurring_nonrecurring)
{
    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
                &getCanBuses()[0], &request, "foo", false, false, 1, 0, NULL, NULL, 1));
    // get around the staggered start
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    FAKE_TIME += 2000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    resetQueues();

    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
                &getCanBuses()[0], &request, "bar", false, false));

    FAKE_TIME += 50;
    // the recurring is in flight and not expired, so the new non-recurring
    // should *not* send.
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));

    resetQueues();

    FAKE_TIME += 50;
    // the recurring is now expired, so we clean it up and then are clear to
    // send the non-recurring
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    // satisfy the non-recurring request, which is the only request in flight
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot, "foo") == NULL);
    ck_assert(strstr((char*)snapshot, "bar") != NULL);

    resetQueues();

    // add another non-recurring
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
                &getCanBuses()[0], &request, "bar", false, false));

    // send again, but now non-recurring should send first
    FAKE_TIME += 1000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot, "foo") == NULL);
    ck_assert(strstr((char*)snapshot, "bar") != NULL);

    resetQueues();

    // finally send again and recurring will go through
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot, "foo") != NULL);
    ck_assert(strstr((char*)snapshot, "bar") == NULL);
}
END_TEST

START_TEST (test_cancel_recurring)
{
    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 1));
    // get around the staggered start
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    FAKE_TIME += 2000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    ck_assert(diagnostics::cancelRecurringRequest(
             &getConfiguration()->diagnosticsManager, &getCanBuses()[0],
             &request));

    resetQueues();
    FAKE_TIME += 1000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_add_nonrecurring_doesnt_clobber_recurring)
{
    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 1));
    // get around the staggered start
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    FAKE_TIME += 2000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    resetQueues();

    ck_assert(diagnostics::addRequest(
             &getConfiguration()->diagnosticsManager, &getCanBuses()[0],
             &request));

    FAKE_TIME += 1000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    FAKE_TIME += 1000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
}
END_TEST

START_TEST (test_add_basic_request)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"bus\":1,\"id\":2016,\"mode\":1,\"success\":true,\"pid\":2,\"payload\":\"0x45\"}\r\n");
}
END_TEST

START_TEST (test_padding_on_by_default)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
                &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.length, 8);
}
END_TEST

START_TEST (test_padding_enabled)
{
    request.no_frame_padding = false;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
                &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.length, 8);
}
END_TEST

START_TEST (test_padding_disabled)
{
    request.no_frame_padding = true;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
                &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = QUEUE_POP(CanMessage, &getCanBuses()[0].sendQueue);
    ck_assert_int_eq(message.length, 3);
}
END_TEST

START_TEST (test_add_request_other_bus)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
                &getCanBuses()[1], &request, "mypid", false, false));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[1]);
    fail_if(canQueueEmpty(1));
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[1],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"mypid\",\"value\":69}\r\n");
}
END_TEST

START_TEST (test_add_request_with_name)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, "mypid", false, false));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"mypid\",\"value\":69}\r\n");
}
END_TEST

START_TEST (test_scaling)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, "mypid", false, false, 2.0, 14));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"mypid\",\"value\":152}\r\n");
}
END_TEST

static float decodeFloatTimes2(const DiagnosticResponse* response,
        float parsed_payload) {
    return parsed_payload * 2;
}

START_TEST (test_add_request_with_decoder_no_name_allowed)
{
    fail_unless(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, NULL, false, false, 1, 0, decodeFloatTimes2, NULL));
}
END_TEST

static ActiveDiagnosticRequest CALLBACK_REQUEST = {0};
static DiagnosticResponse CALLBACK_RESPONSE = {0};
void myCallback(DiagnosticsManager* manager,
        const ActiveDiagnosticRequest* request,
        const DiagnosticResponse* response,
        float parsed_payload) {
   ck_assert(manager == &getConfiguration()->diagnosticsManager);
   CALLBACK_REQUEST = *request;
   CALLBACK_RESPONSE = *response;
}

START_TEST (test_recurring_obd2_build)
{
    getConfiguration()->recurringObd2Requests = true;
    getConfiguration()->obd2BusAddress = 1;
    initializeVehicleInterface();
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    // should have requested ignition status
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_request_callback)
{
    fail_unless(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, NULL, false, false, 1, 0, NULL, myCallback));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    ck_assert(CALLBACK_REQUEST.arbitration_id == request.arbitration_id);

    ck_assert(CALLBACK_RESPONSE.arbitration_id == request.arbitration_id + 0x8);
    ck_assert(CALLBACK_RESPONSE.mode == request.mode);
    ck_assert(CALLBACK_RESPONSE.pid == request.pid);
}
END_TEST

START_TEST (test_add_request_with_name_and_decoder)
{
    fail_unless(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, "mypid", false, false, 1, 0, decodeFloatTimes2, NULL));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"mypid\",\"value\":138}\r\n");
}
END_TEST

START_TEST (test_nonrecurring_not_staggered)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
             &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    resetQueues();
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST (test_recurring_staggered)
{
    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 1));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
    FAKE_TIME += 2000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST (test_add_recurring)
{
    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 1));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    // get around the staggered start
    FAKE_TIME += 2000;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    resetQueues();

    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));

    FAKE_TIME += 900;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
    FAKE_TIME += 100;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
}
END_TEST

START_TEST (test_receive_nonrecurring_twice)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    // the request should be moved from inflight to active at this point

    resetQueues();

    // the non-recurring request should already be completed, so this should
    // *not* send it again
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_unless(outputQueueEmpty());
}
END_TEST

START_TEST (test_nonrecurring_timeout)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    resetQueues();

    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));

    FAKE_TIME += 100;

    // the request timed out and it's non-recurring, so it should *not* be sent
    // again
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
}
END_TEST

START_TEST(test_clear_to_send_blocked)
{
    // add 2 requests for 2 pids from same arb id. send one request, then the other -
    // confirm no can messages sent.
    // increase time to time out first, then send again - make sure the
    // other pid goes out
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    resetQueues();

    FAKE_TIME += 50;

    request.pid = request.pid + 1;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));

    FAKE_TIME += 100;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
}
END_TEST

START_TEST(test_clear_to_send)
{
    // add 2 requests for 2 pids from same arb id. send one request, then the
    // other - confirm no can messages sent. rx can message to close out first,
    // then send and make sure the other goes out
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    resetQueues();

    // should have 1 in flight
    request.pid = request.pid + 1;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
    resetQueues();
    // should have 1 in flight, 1 active

    // should be expecting PID 2
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
    // should have 0 in flight, 1 active

    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    // should have 1 in flight, 0 active
}
END_TEST

START_TEST(test_command_single_response_default)
{
    openxc_ControlCommand command = {0};
    command.has_type = true;
    command.type = openxc_ControlCommand_Type_DIAGNOSTIC;
    command.has_diagnostic_request = true;
    command.diagnostic_request.has_bus = true;
    command.diagnostic_request.bus = 1;
    command.diagnostic_request.has_message_id = true;
    command.diagnostic_request.message_id = request.arbitration_id;
    command.diagnostic_request.has_mode = true;
    command.diagnostic_request.mode = request.mode;
    command.diagnostic_request.has_pid = true;
    command.diagnostic_request.pid = request.pid;

    ck_assert(diagnostics::handleDiagnosticCommand(
             &getConfiguration()->diagnosticsManager, &command));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = {
         id: 0x7e8,
         data: __builtin_bswap64(0x341024500000000),
         length: 8
    };
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
    resetQueues();
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_unless(outputQueueEmpty());
}
END_TEST

START_TEST(test_command_multiple_responses_default_broadcast)
{
    openxc_ControlCommand command = {0};
    command.has_type = true;
    command.type = openxc_ControlCommand_Type_DIAGNOSTIC;
    command.has_diagnostic_request = true;
    command.diagnostic_request.has_bus = true;
    command.diagnostic_request.bus = 1;
    command.diagnostic_request.has_message_id = true;
    command.diagnostic_request.message_id = OBD2_FUNCTIONAL_BROADCAST_ID;
    command.diagnostic_request.has_mode = true;
    command.diagnostic_request.mode = request.mode;
    command.diagnostic_request.has_pid = true;
    command.diagnostic_request.pid = request.pid;

    ck_assert(diagnostics::handleDiagnosticCommand(
             &getConfiguration()->diagnosticsManager, &command));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = {
         id: 0x7e8,
         data: __builtin_bswap64(0x341024500000000),
         length: 8
    };
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
    resetQueues();
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
}
END_TEST

START_TEST(test_command_multiple_responses)
{
    openxc_ControlCommand command = {0};
    command.has_type = true;
    command.type = openxc_ControlCommand_Type_DIAGNOSTIC;
    command.has_diagnostic_request = true;
    command.diagnostic_request.has_bus = true;
    command.diagnostic_request.bus = 1;
    command.diagnostic_request.has_message_id = true;
    command.diagnostic_request.message_id = request.arbitration_id;
    command.diagnostic_request.has_mode = true;
    command.diagnostic_request.mode = request.mode;
    command.diagnostic_request.has_pid = true;
    command.diagnostic_request.pid = request.pid;
    command.diagnostic_request.has_multiple_responses = true;
    command.diagnostic_request.multiple_responses = true;

    ck_assert(diagnostics::handleDiagnosticCommand(
             &getConfiguration()->diagnosticsManager, &command));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = {
         id: 0x7e8,
         data: __builtin_bswap64(0x341024500000000),
         length: 8
    };
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
    resetQueues();
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
}
END_TEST

START_TEST(test_broadcast_accept_multiple_responses)
{
    request.arbitration_id = OBD2_FUNCTIONAL_BROADCAST_ID;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, NULL, false, true, 1, 0));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = {
         id: 0x7e8,
         data: __builtin_bswap64(0x341024500000000),
         length: 8
    };
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
    resetQueues();
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

}
END_TEST

START_TEST(test_broadcast_response_arb_id)
{
    // send a broadcast request, rx a response. make sure the response's message
    // ID is the response's message ID, not the func. broadcast ID
    request.arbitration_id = OBD2_FUNCTIONAL_BROADCAST_ID;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = {
         id: 0x7e8,
         data: __builtin_bswap64(0x341024500000000),
         length: 8
    };
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot, "2024") != NULL);
    ck_assert(strstr((char*)snapshot, "2015") == NULL);
}
END_TEST

START_TEST(test_parsed_payload)
{
    ck_assert(diagnostics::addRequest(
             &getConfiguration()->diagnosticsManager,
             &getCanBuses()[0], &request, NULL, true, false));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    CanMessage message = {
         id: 0x7e8,
         data: __builtin_bswap64(0x341024500000000),
         length: 8
    };
    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert(strstr((char*)snapshot, "69") != NULL);
}
END_TEST

START_TEST(test_requests_on_multiple_buses)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    resetQueues();

    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[1], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[1]);

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[0],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
    resetQueues();

    diagnostics::receiveCanMessage(&getConfiguration()->diagnosticsManager, &getCanBuses()[1],
            &message, &getConfiguration()->pipeline);
    fail_if(outputQueueEmpty());
}
END_TEST

START_TEST(test_update_inflight)
{
    // Add and send one diag request, then before rx or timeout, update it.
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    resetQueues();

    ck_assert(diagnostics::addRecurringRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request, 10));
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    FAKE_TIME += 100;
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    resetQueues();
    FAKE_TIME += 100;

    // recur
    diagnostics::sendRequests(&getConfiguration()->diagnosticsManager, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
}

END_TEST

START_TEST(test_use_all_free_entries)
{
    for(int i = 0; i < MAX_SIMULTANEOUS_DIAG_REQUESTS; i++) {
        request.arbitration_id = 1 + i;
        ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
                &getCanBuses()[0], &request));
    }
    ++request.arbitration_id;
    ck_assert(!diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
}
END_TEST

int countFilters(CanBus* bus) {
    int filterCount = 0;
    AcceptanceFilterListEntry* entry;
    LIST_FOREACH(entry, &bus->acceptanceFilters, entries) {
        ++filterCount;
    }
    return filterCount;
}

START_TEST(test_broadcast_can_filters)
{
    request.arbitration_id = OBD2_FUNCTIONAL_BROADCAST_ID;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));

    ck_assert_int_eq(countFilters(&getCanBuses()[0]), 8);
}
END_TEST

START_TEST(test_can_filters)
{
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    ck_assert_int_eq(countFilters(&getCanBuses()[0]), 1);

    request.pid = request.pid + 1;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));

    ck_assert_int_eq(countFilters(&getCanBuses()[0]), 1);

    ++request.arbitration_id;
    ck_assert(diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    ck_assert_int_eq(countFilters(&getCanBuses()[0]), 2);

}
END_TEST

START_TEST(test_can_filters_broken)
{
    ACCEPTANCE_FILTER_STATUS = false;
    ck_assert(!diagnostics::addRequest(&getConfiguration()->diagnosticsManager,
            &getCanBuses()[0], &request));
    ck_assert_int_eq(countFilters(&getCanBuses()[0]), 0);
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("diagnostics");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_add_basic_request);
    tcase_add_test(tc_core, test_add_request_other_bus);
    tcase_add_test(tc_core, test_add_request_with_name);
    tcase_add_test(tc_core, test_add_request_with_decoder_no_name_allowed);
    tcase_add_test(tc_core, test_add_request_with_name_and_decoder);
    tcase_add_test(tc_core, test_add_recurring);
    tcase_add_test(tc_core, test_add_recurring_too_frequent);
    tcase_add_test(tc_core, test_padding_on_by_default);
    tcase_add_test(tc_core, test_padding_enabled);
    tcase_add_test(tc_core, test_padding_disabled);
    tcase_add_test(tc_core, test_scaling);
    tcase_add_test(tc_core, test_update_existing_recurring);
    tcase_add_test(tc_core, test_simultaneous_recurring_nonrecurring);
    tcase_add_test(tc_core, test_cancel_recurring);
    tcase_add_test(tc_core, test_add_nonrecurring_doesnt_clobber_recurring);
    tcase_add_test(tc_core, test_receive_nonrecurring_twice);
    tcase_add_test(tc_core, test_nonrecurring_timeout);

    tcase_add_test(tc_core, test_recurring_staggered);
    tcase_add_test(tc_core, test_nonrecurring_not_staggered);

    tcase_add_test(tc_core, test_clear_to_send_blocked);
    tcase_add_test(tc_core, test_clear_to_send);
    tcase_add_test(tc_core, test_broadcast_response_arb_id);
    tcase_add_test(tc_core, test_broadcast_accept_multiple_responses);
    tcase_add_test(tc_core, test_parsed_payload);
    tcase_add_test(tc_core, test_requests_on_multiple_buses);
    tcase_add_test(tc_core, test_update_inflight);
    tcase_add_test(tc_core, test_use_all_free_entries);
    tcase_add_test(tc_core, test_broadcast_can_filters);
    tcase_add_test(tc_core, test_can_filters);
    tcase_add_test(tc_core, test_can_filters_broken);

    tcase_add_test(tc_core, test_command_multiple_responses);
    tcase_add_test(tc_core, test_command_multiple_responses_default_broadcast);
    tcase_add_test(tc_core, test_command_single_response_default);

    tcase_add_test(tc_core, test_request_callback);

    tcase_add_test(tc_core, test_recurring_obd2_build);

    suite_add_tcase(s, tc_core);

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
