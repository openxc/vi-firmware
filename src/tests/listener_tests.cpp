#include <check.h>
#include <stdint.h>
#include "interface/pipeline.h"
#include "emqueue.h"
#include "cJSON.h"

using openxc::interface::Listener;

Listener listener;
UsbDevice usb;
SerialDevice serial;
NetworkDevice network;

extern bool USB_PROCESSED;
extern bool SERIAL_PROCESSED;
extern bool NETWORK_PROCESSED;

void setup() {
    listener.usb = &usb;
    listener.serial = NULL;
    listener.network = NULL;
    initializeUsb(&usb);
    initializeSerial(&serial);
    initializeNetwork(&network);
    listener.usb->configured = true;
    USB_PROCESSED = false;
    SERIAL_PROCESSED = false;
    NETWORK_PROCESSED = false;
}

START_TEST (test_only_usb)
{
    const char* message = "message";
    sendMessage(&listener, (uint8_t*)message, 8);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_full_network)
{
    listener.network = &network;
    for(int i = 0; i < 512; i++) {
        QUEUE_PUSH(uint8_t, &listener.network->sendQueue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &listener.network->sendQueue));

    const char* message = "message";
    sendMessage(&listener, (uint8_t*)message, 8);
}
END_TEST

START_TEST (test_full_uart)
{
    listener.serial = &serial;
    for(int i = 0; i < 512; i++) {
        QUEUE_PUSH(uint8_t, &listener.serial->sendQueue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &listener.serial->sendQueue));

    const char* message = "message";
    sendMessage(&listener, (uint8_t*)message, 8);
}
END_TEST

START_TEST (test_full_usb)
{
    for(int i = 0; i < 512; i++) {
        QUEUE_PUSH(uint8_t, &listener.usb->sendQueue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &listener.usb->sendQueue));

    const char* message = "message";
    sendMessage(&listener, (uint8_t*)message, 8);
}
END_TEST

START_TEST (test_with_uart)
{
    listener.serial = &serial;
    const char* message = "message";
    sendMessage(&listener, (uint8_t*)message, 8);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");

    snapshot[QUEUE_LENGTH(uint8_t, &listener.serial->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &listener.serial->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_with_uart_and_network)
{
    listener.serial = &serial;
    listener.network = &network;
    const char* message = "message";
    sendMessage(&listener, (uint8_t*)message, 8);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &listener.usb->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &listener.usb->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");

    snapshot[QUEUE_LENGTH(uint8_t, &listener.serial->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &listener.serial->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");

    snapshot[QUEUE_LENGTH(uint8_t, &listener.network->sendQueue)];
    QUEUE_SNAPSHOT(uint8_t, &listener.network->sendQueue, snapshot);
    ck_assert_str_eq((char*)snapshot, "message");
}
END_TEST

START_TEST (test_process_usb)
{
    processListenerQueues(&listener);
    fail_unless(USB_PROCESSED);
    fail_if(SERIAL_PROCESSED);
    fail_if(NETWORK_PROCESSED);
}
END_TEST

START_TEST (test_process_usb_and_uart)
{
    listener.serial = &serial;
    processListenerQueues(&listener);
    fail_unless(USB_PROCESSED);
    fail_unless(SERIAL_PROCESSED);
    fail_if(NETWORK_PROCESSED);
}
END_TEST

START_TEST (test_process_all)
{
    listener.serial = &serial;
    listener.network = &network;
    processListenerQueues(&listener);
    fail_unless(USB_PROCESSED);
    fail_unless(SERIAL_PROCESSED);
    fail_unless(NETWORK_PROCESSED);
}
END_TEST

Suite* listenerSuite(void) {
    Suite* s = suite_create("listener");
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
    Suite* s = listenerSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
