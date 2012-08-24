#include <check.h>
#include <stdint.h>
#include "queue.h"
#include <stdio.h>

START_TEST (test_push)
{
    QUEUE_TYPE(uint8_t) queue;
    queue_init(&queue);
    int length = queue_length(&queue);
    fail_unless(length == 0, "expected queue length of 0 but got %d", length);
    bool success = QUEUE_PUSH(uint8_t, &queue, 0xEF);
    fail_unless(success);
    fail_unless(queue_length(&queue) == 1,
            "expected queue length of 1 but got %d", length);
}
END_TEST

START_TEST (test_pop)
{
    QUEUE_TYPE(uint8_t) queue;
    queue_init(&queue);
    uint8_t original_value = 0xEF;
    QUEUE_PUSH(uint8_t, &queue, original_value);
    uint8_t value = QUEUE_POP(uint8_t, &queue);
    fail_unless(value == original_value);
}
END_TEST

START_TEST (test_fill_er_up)
{
    QUEUE_TYPE(uint8_t) queue;
    queue_init(&queue);
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t); i++) {
        bool success = QUEUE_PUSH(uint8_t, &queue, (uint8_t) (i % 255));
        fail_unless(success, "wasn't able to add the %dth element", i + 1);
    }

    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t); i++) {
        uint8_t value = QUEUE_POP(uint8_t, &queue);
        if(i < QUEUE_MAX_LENGTH(uint8_t) - 1) {
            fail_unless(!queue_empty(&queue),
                    "didn't expect queue to be empty on %dth iteration", i + 1);
        }
        uint8_t expected = i % 255;
        fail_unless(value == expected,
                "expected %d but got %d out of the queue", expected, value);
    }
    fail_unless(queue_empty(&queue));
}
END_TEST

START_TEST (test_length)
{
    QUEUE_TYPE(uint8_t) queue;
    queue_init(&queue);
    fail_unless(queue_length(&queue) == 0);
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t); i++) {
        QUEUE_PUSH(uint8_t, &queue,  (uint8_t) (i % 255));
        if(i == QUEUE_MAX_LENGTH(uint8_t) - 1) {
            break;
        }
        fail_unless(queue_length(&queue) == i + 1,
                "expected length of %d but found %d", i + 1,
                queue_length(&queue));
    }

    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t); i++) {
        QUEUE_POP(uint8_t, &queue);
        fail_unless(queue_length(&queue) == QUEUE_MAX_LENGTH(uint8_t) - i - 1);
    }
    fail_unless(queue_length(&queue) == 0);

    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t); i++) {
        QUEUE_PUSH(uint8_t, &queue, (uint8_t) (i % 255));
        fail_unless(queue_length(&queue) == i + 1,
                "expected length of %d but found %d", i + 1,
                queue_length(&queue));
    }

    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) / 2; i++) {
        QUEUE_POP(uint8_t, &queue);
        fail_unless(queue_length(&queue) == QUEUE_MAX_LENGTH(uint8_t) - i - 1);
    }

    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t) / 2; i++) {
        QUEUE_PUSH(uint8_t, &queue, (uint8_t) (i % 255));
        int expectedLength =  i + (QUEUE_MAX_LENGTH(uint8_t) / 2) + 1;
        fail_unless(queue_length(&queue) == expectedLength,
                "expected length of %d but found %d", expectedLength,
                queue_length(&queue));
    }
}
END_TEST

START_TEST (test_snapshot)
{
    QUEUE_TYPE(uint8_t) queue;
    queue_init(&queue);
    uint8_t expected[QUEUE_MAX_LENGTH(uint8_t)];
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t); i++) {
        uint8_t value = i % 255;
        QUEUE_PUSH(uint8_t, &queue, value);
        expected[i] = value;
    }

    uint8_t snapshot[QUEUE_MAX_LENGTH(uint8_t)];
    queue_snapshot(&queue, snapshot);
    for(int i = 0; i < QUEUE_MAX_LENGTH(uint8_t); i++) {
        fail_unless(snapshot[i] == expected[i],
                "expected %d but found %d", expected[i], snapshot[i]);
    }
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("queue");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_push);
    tcase_add_test(tc_core, test_pop);
    tcase_add_test(tc_core, test_fill_er_up);
    tcase_add_test(tc_core, test_length);
    tcase_add_test(tc_core, test_snapshot);
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
