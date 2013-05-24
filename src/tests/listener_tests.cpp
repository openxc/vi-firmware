#include <check.h>
#include <stdint.h>
#include "interface/pipeline.h"
#include "emqueue.h"
#include "cJSON.h"

using openxc::interface::Pipeline;

Pipeline pipeline;
UsbDevice usb;
UartDevice uart;
NetworkDevice network;

extern bool USB_PROCESSED;
extern bool UART_PROCESSED;
extern bool NETWORK_PROCESSED;

void setup() {
    pipeline.usb = &usb;
    pipeline.uart = NULL;
    pipeline.network = NULL;
    initializeUsb(&usb);
    initializeUart(&uart);
    initializeNetwork(&network);
    pipeline.usb->configured = true;
    USB_PROCESSED = false;
    UART_PROCESSED = false;
    NETWORK_PROCESSED = false;
}

START_TEST (test_only_usb)
{
    const char* message = "message";
    sendMessage(&pipeline, (uint8_t*)message, 8);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &pipeline.usb->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &pipeline.usb->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_full_network)
{
    pipeline.network = &network;
    for(int i = 0; i < 512; i++) {
        QUEUE_PUSH(uint8_t, &pipeline.network->sendQueue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &pipeline.network->sendQueue));

    const char* message = "message";
    sendMessage(&pipeline, (uint8_t*)message, 8);
}
END_TEST

START_TEST (test_full_uart)
{
    pipeline.uart = &uart;
    for(int i = 0; i < 512; i++) {
        QUEUE_PUSH(uint8_t, &pipeline.uart->sendQueue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &pipeline.uart->sendQueue));

    const char* message = "message";
    sendMessage(&pipeline, (uint8_t*)message, 8);
}
END_TEST

START_TEST (test_full_usb)
{
    for(int i = 0; i < 512; i++) {
        QUEUE_PUSH(uint8_t, &pipeline.usb->sendQueue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &pipeline.usb->sendQueue));

    const char* message = "message";
    sendMessage(&pipeline, (uint8_t*)message, 8);
}
END_TEST

START_TEST (test_with_uart)
{
    pipeline.uart = &uart;
    const char* message = "message";
    sendMessage(&pipeline, (uint8_t*)message, 8);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &pipeline.usb->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &pipeline.usb->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");

    snapshot[QUEUE_LENGTH(uint8_t, &pipeline.uart->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &pipeline.uart->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_with_uart_and_network)
{
    pipeline.uart = &uart;
    pipeline.network = &network;
    const char* message = "message";
    sendMessage(&pipeline, (uint8_t*)message, 8);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &pipeline.usb->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &pipeline.usb->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");

    snapshot[QUEUE_LENGTH(uint8_t, &pipeline.uart->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &pipeline.uart->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");

    snapshot[QUEUE_LENGTH(uint8_t, &pipeline.network->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &pipeline.network->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_process_usb)
{
    processPipelineQueues(&pipeline);
    fail_unless(USB_PROCESSED);
    fail_if(UART_PROCESSED);
    fail_if(NETWORK_PROCESSED);
}
END_TEST

START_TEST (test_process_usb_and_uart)
{
    pipeline.uart = &uart;
    processPipelineQueues(&pipeline);
    fail_unless(USB_PROCESSED);
    fail_unless(UART_PROCESSED);
    fail_if(NETWORK_PROCESSED);
}
END_TEST

START_TEST (test_process_all)
{
    pipeline.uart = &uart;
    pipeline.network = &network;
    processPipelineQueues(&pipeline);
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
