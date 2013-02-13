#include <check.h>
#include <stdint.h>
#include "buffers.h"

ByteQueue queue;
bool called;
bool callbackStatus;

void setup() {
    QUEUE_INIT(uint8_t, &queue);
    called = false;
    callbackStatus = false;
}

void teardown() {
}

bool callback(uint8_t* message) {
    called = true;
    return callbackStatus;
}

START_TEST (test_empty_doesnt_call)
{
    processQueue(&queue, callback);
    fail_if(called);
}
END_TEST

START_TEST (test_missing_callback)
{
    QUEUE_PUSH(uint8_t, &queue, (uint8_t) 128);
    processQueue(&queue, NULL);
    fail_if(called);
    fail_if(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_success_clears)
{
    callbackStatus = true;
    QUEUE_PUSH(uint8_t, &queue, (uint8_t) 128);
    processQueue(&queue, callback);
    fail_unless(called);
    fail_unless(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_failure_preserves)
{
    callbackStatus = false;
    QUEUE_PUSH(uint8_t, &queue, (uint8_t) 128);
    processQueue(&queue, callback);
    fail_unless(called);
    fail_if(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_clear_corrupted)
{
    callbackStatus = false;
    QUEUE_PUSH(uint8_t, &queue, (uint8_t) 128);
    QUEUE_PUSH(uint8_t, &queue, (uint8_t) '\0');
    processQueue(&queue, callback);
    fail_unless(called);
    fail_unless(QUEUE_EMPTY(uint8_t, &queue));
}
END_TEST

START_TEST (test_full_clears)
{
    for(int i = 0; i < 512; i++) {
        QUEUE_PUSH(uint8_t, &queue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &queue));

    callbackStatus = false;
    processQueue(&queue, callback);
    fail_unless(called);
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
    for(int i = 0; i < 512; i++) {
        QUEUE_PUSH(uint8_t, &queue, (uint8_t) 128);
    }
    fail_unless(QUEUE_FULL(uint8_t, &queue));

    char* message = "a message";
    bool result = conditionalEnqueue(&queue, (uint8_t*)message, 10);
    fail_if(result);
}
END_TEST

START_TEST (test_enqueue_just_enough_room)
{
    for(int i = 0; i < 501; i++) {
        QUEUE_PUSH(uint8_t, &queue, (uint8_t) 128);
    }

    char* message = "a message";
    bool result = conditionalEnqueue(&queue, (uint8_t*)message, 9);
    fail_unless(result);
}
END_TEST

START_TEST (test_enqueue_no_room_for_crlf)
{
    for(int i = 0; i < 503; i++) {
        QUEUE_PUSH(uint8_t, &queue, (uint8_t) 128);
    }

    char* message = "a message";
    bool result = conditionalEnqueue(&queue, (uint8_t*)message, 9);
    fail_if(result);
}
END_TEST

START_TEST (test_enqueue_adds_crlf)
{
    char* message = "a message";
    bool result = conditionalEnqueue(&queue, (uint8_t*)message, 9);
    fail_unless(result);
    ck_assert_int_eq(QUEUE_LENGTH(uint8_t, &queue), 11);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, &queue)];
    QUEUE_SNAPSHOT(uint8_t, &queue, snapshot);
    const char* expected = "a message\r\n";
    for(int i = 0; i < 11; i++) {
        fail_unless((char)snapshot[i] == expected[i]);
    }
}
END_TEST

Suite* buffersSuite(void) {
    Suite* s = suite_create("buffers");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture (tc_core, setup, teardown);
    tcase_add_test(tc_core, test_empty_doesnt_call);
    tcase_add_test(tc_core, test_success_clears);
    tcase_add_test(tc_core, test_failure_preserves);
    tcase_add_test(tc_core, test_clear_corrupted);
    tcase_add_test(tc_core, test_full_clears);
    tcase_add_test(tc_core, test_missing_callback);
    suite_add_tcase(s, tc_core);

    TCase *tc_conditional = tcase_create("conditional");
    tcase_add_checked_fixture (tc_conditional, setup, teardown);
    tcase_add_test(tc_conditional, test_null_queue);
    tcase_add_test(tc_conditional, test_enqueue_empty);
    tcase_add_test(tc_conditional, test_enqueue_full);
    tcase_add_test(tc_conditional, test_enqueue_no_room_for_crlf);
    tcase_add_test(tc_conditional, test_enqueue_adds_crlf);
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
