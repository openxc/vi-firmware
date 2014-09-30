#include <check.h>
#include <stdint.h>
#include "util/bytebuffer.h"

using openxc::util::bytebuffer::conditionalEnqueue;
using openxc::util::bytebuffer::processQueue;

QUEUE_TYPE(uint8_t) queue;
bool called;
size_t callbackDataRead;
int calledTimes;

void setup() {
    QUEUE_INIT(uint8_t, &queue);
    called = false;
    callbackDataRead = 0;
    calledTimes = 0;
}

void teardown() {
}

uint8_t received_message[8];
size_t callback(uint8_t* message, size_t length) {
    called = true;
    calledTimes++;
    memcpy(received_message, message, length);
    return callbackDataRead;
}

START_TEST (test_empty_doesnt_call)
{
    processQueue(&queue, callback);
    fail_if(called);
}
END_TEST

START_TEST (test_missing_callback)
{
    QUEUE_PUSH(uint8_t, &queue, 128);
    processQueue(&queue, NULL);
    fail_if(called);
    fail_if(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_parse_multiple)
{
    callbackDataRead = 2;
    QUEUE_PUSH(uint8_t, &queue, 128);
    QUEUE_PUSH(uint8_t, &queue, 0);
    QUEUE_PUSH(uint8_t, &queue, 64);
    QUEUE_PUSH(uint8_t, &queue, 0);

    processQueue(&queue, callback);
    processQueue(&queue, callback);
    ck_assert_int_eq(calledTimes, 2);
    fail_unless(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_data_sent_to_callback)
{
    callbackDataRead = 2;
    QUEUE_PUSH(uint8_t, &queue, 128);
    QUEUE_PUSH(uint8_t, &queue, 0);
    processQueue(&queue, callback);
    ck_assert_int_eq(received_message[0], 128);
    ck_assert_int_eq(received_message[1], 0);
}
END_TEST

START_TEST (test_success_clears)
{
    callbackDataRead = 2;
    QUEUE_PUSH(uint8_t, &queue, 128);
    QUEUE_PUSH(uint8_t, &queue, 0);
    processQueue(&queue, callback);
    fail_unless(called);
    fail_unless(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_failure_clears_too)
{
    callbackDataRead = 2;
    QUEUE_PUSH(uint8_t, &queue, 128);
    QUEUE_PUSH(uint8_t, &queue, 0);
    processQueue(&queue, callback);
    fail_unless(called);
    fail_unless(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_full_clears)
{
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) + 1; i++) {
        QUEUE_PUSH(uint8_t, &queue, 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &queue));

    callbackDataRead = 0;
    processQueue(&queue, callback);
    fail_unless(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_null_queue)
{
    char* message = "a message";
    bool result = conditionalEnqueue(NULL, (uint8_t*)message, 10);
    fail_if(result);
}
END_TEST

START_TEST (test_enqueue_empty)
{
    char* message = "a message";
    bool result = conditionalEnqueue(&queue, (uint8_t*)message, 10);
    fail_unless(result);
}
END_TEST

START_TEST (test_enqueue_full)
{
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) + 1; i++) {
        QUEUE_PUSH(uint8_t, &queue, 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &queue));

    char* message = "a message";
    bool result = conditionalEnqueue(&queue, (uint8_t*)message, 10);
    fail_if(result);
}
END_TEST

START_TEST (test_enqueue_just_enough_room)
{
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) - 11; i++) {
        QUEUE_PUSH(uint8_t, &queue, 128);
    }

    char* message = "a message";
    bool result = conditionalEnqueue(&queue, (uint8_t*)message, 9);
    fail_unless(result);
}
END_TEST

START_TEST (test_enqueue_no_room_for_crlf)
{
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) - 9; i++) {
        QUEUE_PUSH(uint8_t, &queue, 128);
    }

    char* message = "a message";
    bool result = conditionalEnqueue(&queue, (uint8_t*)message, 9);
    fail_if(result);
}
END_TEST

Suite* buffersSuite(void) {
    Suite* s = suite_create("buffers");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture (tc_core, setup, teardown);
    tcase_add_test(tc_core, test_empty_doesnt_call);
    tcase_add_test(tc_core, test_success_clears);
    tcase_add_test(tc_core, test_data_sent_to_callback);
    tcase_add_test(tc_core, test_failure_clears_too);
    tcase_add_test(tc_core, test_full_clears);
    tcase_add_test(tc_core, test_missing_callback);
    tcase_add_test(tc_core, test_parse_multiple);
    suite_add_tcase(s, tc_core);

    TCase *tc_conditional = tcase_create("conditional");
    tcase_add_checked_fixture (tc_conditional, setup, teardown);
    tcase_add_test(tc_conditional, test_null_queue);
    tcase_add_test(tc_conditional, test_enqueue_empty);
    tcase_add_test(tc_conditional, test_enqueue_full);
    tcase_add_test(tc_conditional, test_enqueue_no_room_for_crlf);
    tcase_add_test(tc_conditional, test_enqueue_just_enough_room);
    suite_add_tcase(s, tc_conditional);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = buffersSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
