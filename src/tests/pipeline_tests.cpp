#include <check.h>
#include <stdint.h>
#include "pipeline.h"
#include "emqueue.h"
#include "config.h"

namespace uart = openxc::interface::uart;
namespace network = openxc::interface::network;
namespace usb = openxc::interface::usb;

using openxc::pipeline::Pipeline;
using openxc::pipeline::MessageClass;
using openxc::config::getConfiguration;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[IN_ENDPOINT_INDEX].queue;
QUEUE_TYPE(uint8_t)* LOG_QUEUE = &getConfiguration()->usb.endpoints[LOG_ENDPOINT_INDEX].queue;

extern bool USB_PROCESSED;
extern bool UART_PROCESSED;
extern bool NETWORK_PROCESSED;

void setup() {
    getConfiguration()->pipeline.usb = &getConfiguration()->usb;
    getConfiguration()->pipeline.uart = NULL;
    getConfiguration()->pipeline.network = NULL;
    usb::initialize(&getConfiguration()->usb);
    uart::initialize(&getConfiguration()->uart);
    network::initialize(&getConfiguration()->network);
    getConfiguration()->usb.configured = true;
    USB_PROCESSED = false;
    UART_PROCESSED = false;
    NETWORK_PROCESSED = false;
}

START_TEST (test_log_to_usb)
{
    const char* message = "message";
    sendMessage(&getConfiguration()->pipeline, (uint8_t*)message, 8, MessageClass::LOG);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, LOG_QUEUE)];
    ck_assert(QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE));
    ck_assert(!QUEUE_EMPTY(uint8_t, &getConfiguration()->usb.endpoints[LOG_ENDPOINT_INDEX].queue));
    QUEUE_SNAPSHOT(uint8_t, &getConfiguration()->usb.endpoints[LOG_ENDPOINT_INDEX].queue, snapshot, sizeof(snapshot));
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_only_usb)
{
    const char* message = "message";
    sendMessage(&getConfiguration()->pipeline, (uint8_t*)message, 8, MessageClass::SIMPLE);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE)];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_full_network)
{
    getConfiguration()->pipeline.network = &getConfiguration()->network;
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) + 1; i++) {
        QUEUE_PUSH(uint8_t, &getConfiguration()->pipeline.network->sendQueue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &getConfiguration()->pipeline.network->sendQueue));

    const char* message = "message";
    sendMessage(&getConfiguration()->pipeline, (uint8_t*)message, 8, MessageClass::SIMPLE);
}
END_TEST

START_TEST (test_full_uart)
{
    getConfiguration()->pipeline.uart = &getConfiguration()->uart;
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) + 1; i++) {
        QUEUE_PUSH(uint8_t, &getConfiguration()->pipeline.uart->sendQueue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &getConfiguration()->pipeline.uart->sendQueue));

    const char* message = "message";
    sendMessage(&getConfiguration()->pipeline, (uint8_t*)message, 8, MessageClass::SIMPLE);
}
END_TEST

START_TEST (test_full_usb)
{
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) + 1; i++) {
        QUEUE_PUSH(uint8_t, OUTPUT_QUEUE, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, OUTPUT_QUEUE));

    const char* message = "message";
    sendMessage(&getConfiguration()->pipeline, (uint8_t*)message, 8, MessageClass::SIMPLE);
}
END_TEST

START_TEST (test_with_uart)
{
    getConfiguration()->pipeline.uart = &getConfiguration()->uart;
    const char* message = "message";
    sendMessage(&getConfiguration()->pipeline, (uint8_t*)message, 8, MessageClass::SIMPLE);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE)];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    ck_assert_str_eq((char*)snapshot, "message");

    QUEUE_SNAPSHOT(uint8_t, &getConfiguration()->pipeline.uart->sendQueue, snapshot, sizeof(snapshot));
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_with_uart_and_network)
{
    getConfiguration()->pipeline.uart = &getConfiguration()->uart;
    getConfiguration()->pipeline.network = &getConfiguration()->network;
    const char* message = "message";
    sendMessage(&getConfiguration()->pipeline, (uint8_t*)message, 8, MessageClass::SIMPLE);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE)];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    ck_assert_str_eq((char*)snapshot, "message");

    QUEUE_SNAPSHOT(uint8_t, &getConfiguration()->pipeline.uart->sendQueue, snapshot, sizeof(snapshot));
    ck_assert_str_eq((char*)snapshot, "message");

    QUEUE_SNAPSHOT(uint8_t, &getConfiguration()->pipeline.network->sendQueue, snapshot, sizeof(snapshot));
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_process_usb)
{
    process(&getConfiguration()->pipeline);
    fail_unless(USB_PROCESSED);
    fail_if(UART_PROCESSED);
    fail_if(NETWORK_PROCESSED);
}
END_TEST

START_TEST (test_process_usb_and_uart)
{
    getConfiguration()->pipeline.uart = &getConfiguration()->uart;
    process(&getConfiguration()->pipeline);
    fail_unless(USB_PROCESSED);
    fail_unless(UART_PROCESSED);
    fail_if(NETWORK_PROCESSED);
}
END_TEST

START_TEST (test_process_all)
{
    getConfiguration()->pipeline.uart = &getConfiguration()->uart;
    getConfiguration()->pipeline.network = &getConfiguration()->network;
    process(&getConfiguration()->pipeline);
    fail_unless(USB_PROCESSED);
    fail_unless(UART_PROCESSED);
    fail_unless(NETWORK_PROCESSED);
}
END_TEST

Suite* pipelineSuite(void) {
    Suite* s = suite_create("pipeline");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_only_usb);
    tcase_add_test(tc_core, test_with_uart);
    tcase_add_test(tc_core, test_with_uart_and_network);
    tcase_add_test(tc_core, test_full_usb);
    tcase_add_test(tc_core, test_full_uart);
    tcase_add_test(tc_core, test_full_network);
    tcase_add_test(tc_core, test_process_all);
    tcase_add_test(tc_core, test_process_usb_and_uart);
    tcase_add_test(tc_core, test_process_usb);
    tcase_add_test(tc_core, test_log_to_usb);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = pipelineSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
