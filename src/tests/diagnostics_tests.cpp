#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "diagnostics.h"

namespace diagnostics = openxc::diagnostics;
namespace usb = openxc::interface::usb;

using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getMessages;
using openxc::pipeline::Pipeline;

diagnostics::DiagnosticsManager DIAGNOSTICS_MANAGER;
extern Pipeline PIPELINE;
extern UsbDevice USB_DEVICE;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &PIPELINE.usb->endpoints[IN_ENDPOINT_INDEX].queue;

DiagnosticRequest request = {
    arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
    mode: OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST,
    pid: 0x2,
    pid_length: 1
};
CanMessage message = {0x7e8, __builtin_bswap64(0x341024500000000)};

static bool canQueueEmpty(int bus) {
    return QUEUE_EMPTY(CanMessage, &getCanBuses()[bus].sendQueue);
}

bool outputQueueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

void setup() {
    PIPELINE.outputFormat = openxc::pipeline::JSON;
    PIPELINE.usb = &USB_DEVICE;
    usb::initialize(&USB_DEVICE);
    PIPELINE.usb->configured = true;
    for(int i = 0; i < getCanBusCount(); i++) {
        openxc::can::initializeCommon(&getCanBuses()[i]);
    }
    diagnostics::initialize(&DIAGNOSTICS_MANAGER, getCanBuses(),
            getCanBusCount());
    fail_unless(canQueueEmpty(0));
}

START_TEST (test_add_basic_request)
{
    ck_assert(diagnostics::addDiagnosticRequest(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &request, NULL, NULL));
    diagnostics::sendRequests(&DIAGNOSTICS_MANAGER, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &message, &PIPELINE);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"bus\":1,\"id\":2024,\"mode\":1,\"success\":true,\"pid\":2,\"payload\":\"0x45\"}\r\n");
}
END_TEST

START_TEST (test_add_request_with_name)
{
    ck_assert(diagnostics::addDiagnosticRequest(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &request, "mypid", NULL));
    diagnostics::sendRequests(&DIAGNOSTICS_MANAGER, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &message, &PIPELINE);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"mypid\",\"value\":69}\r\n");
}
END_TEST

static float decodeFloatTimes2(const DiagnosticResponse* response) {
    float value = diagnostic_payload_to_float(response);
    return value * 2;
}

START_TEST (test_add_request_with_decoder_no_name)
{
    fail_if(diagnostics::addDiagnosticRequest(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &request, NULL, decodeFloatTimes2));
}
END_TEST

START_TEST (test_add_request_with_name_and_decoder)
{
    fail_unless(diagnostics::addDiagnosticRequest(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &request, "mypid", decodeFloatTimes2));
    diagnostics::sendRequests(&DIAGNOSTICS_MANAGER, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &message, &PIPELINE);
    fail_if(outputQueueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"mypid\",\"value\":138}\r\n");
}
END_TEST

START_TEST (test_add_request_with_frequency)
{
    ck_assert(diagnostics::addDiagnosticRequest(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &request, NULL, NULL, 1));
    diagnostics::sendRequests(&DIAGNOSTICS_MANAGER, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &message, &PIPELINE);
    fail_if(outputQueueEmpty());

    openxc::can::initializeCommon(&getCanBuses()[0]);
    usb::initialize(&USB_DEVICE);
    fail_unless(canQueueEmpty(0));
    fail_unless(outputQueueEmpty());
    // TODO expire timer...no good way to do that without reaching really deep
    // into the library internals

    // diagnostics::sendRequests(&DIAGNOSTICS_MANAGER, &getCanBuses()[0]);
    // fail_if(canQueueEmpty(0));
    // diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            // &message, &PIPELINE);
    // fail_if(outputQueueEmpty());
}
END_TEST

START_TEST (test_receive_singletimer_twice)
{
    ck_assert(diagnostics::addDiagnosticRequest(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &request, NULL, NULL));
    diagnostics::sendRequests(&DIAGNOSTICS_MANAGER, &getCanBuses()[0]);
    fail_if(canQueueEmpty(0));

    diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &message, &PIPELINE);
    fail_if(outputQueueEmpty());

    openxc::can::initializeCommon(&getCanBuses()[0]);
    usb::initialize(&USB_DEVICE);
    fail_unless(canQueueEmpty(0));
    fail_unless(outputQueueEmpty());

    diagnostics::sendRequests(&DIAGNOSTICS_MANAGER, &getCanBuses()[0]);
    fail_unless(canQueueEmpty(0));
    diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &message, &PIPELINE);
    fail_unless(outputQueueEmpty());

}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("diagnostics");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_add_basic_request);
    tcase_add_test(tc_core, test_receive_singletimer_twice);
    tcase_add_test(tc_core, test_add_request_with_name);
    tcase_add_test(tc_core, test_add_request_with_decoder_no_name);
    tcase_add_test(tc_core, test_add_request_with_name_and_decoder);
    tcase_add_test(tc_core, test_add_request_with_frequency);

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
