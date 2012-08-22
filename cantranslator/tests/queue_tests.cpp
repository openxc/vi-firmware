#include <check.h>
#include <stdint.h>
#include "queue.h"

START_TEST (test_put)
{
    ByteQueue queue;
    queue_init(&queue);
    int length = queue_length(&queue);
    fail_unless(length == 0, "expected queue length of 0 but got %d", length);
    bool success = queue_put(&queue, 0xEF);
    fail_unless(success);
    fail_unless(queue_length(&queue) == 1);
}
END_TEST

START_TEST (test_get)
{
    ByteQueue queue;
    queue_init(&queue);
    uint8_t original_value = 0xEF;
    queue_put(&queue, original_value);
    uint8_t value = queue_get(&queue);
    fail_unless(value == original_value);
}
END_TEST

START_TEST (test_fill_er_up)
{
    ByteQueue queue;
    queue_init(&queue);
    for(uint8_t i = 0; i < MAX_QUEUE_LENGTH; i++) {
        bool success = queue_put(&queue, i);
        fail_unless(success, "wasn't able to add the %dth element", i + 1);
    }

    for(uint8_t i = 0; i < MAX_QUEUE_LENGTH; i++) {
        uint8_t value = queue_get(&queue);
        if(i < MAX_QUEUE_LENGTH - 1) {
            fail_unless(!queue_empty(&queue),
                    "didn't expect queue to be empty on %dth iteration", i + 1);
        }
        fail_unless(value == i, "expected %d but got %d out of the queue",
                i, value);
    }
    fail_unless(queue_empty(&queue));
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("queue");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_put);
    tcase_add_test(tc_core, test_get);
    tcase_add_test(tc_core, test_fill_er_up);
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
