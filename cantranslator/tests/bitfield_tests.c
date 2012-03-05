#include <check.h>
#include <stdint.h>
#include "bitfield.h"

START_TEST (test_one_byte)
{
    uint8_t data = 0xFA;
    unsigned long result = getBitField(&data, 0, 4);
    fail_unless(result == 0xF,
            "First 4 bits in 0x%X was 0x%X instead of 0xF", data, result);
    result = getBitField(&data, 4, 4);
    fail_unless(result == 0xA,
            "First 4 bits in 0x%X was 0x%X instead of 0xA", data, result);
    result = getBitField(&data, 0, 8);
    fail_unless(result == 0xFA,
            "All bits in 0x%X were 0x%X instead of 0x%X", data, result, data);
}
END_TEST

START_TEST (test_multi_byte)
{
    uint8_t data[2] = {0xFA, 0x12};
    unsigned long result = getBitField(data, 0, 4);
    fail_unless(result == 0xF,
            "First 4 bits in 0x%X was %d instead of 0xF", data[0], result);
    result = getBitField(data, 4, 4);
    fail_unless(result == 0xA,
            "Second 4 bits in 0x%X was %d instead of 0xA", data[0], result);
    result = getBitField(data, 8, 4);
    fail_unless(result == 0x1,
            "First 4 bits in 0x%X was %d instead of 0x1", data[1], result);
    result = getBitField(data, 12, 4);
    fail_unless(result == 0x2,
            "Second 4 bits in 0x%X was %d instead of 0x2", data[1], result);
}
END_TEST

START_TEST (test_get_multi_byte)
{
    uint8_t data[2] = {0xFA, 0x12};
    unsigned long result = getBitField(data, 0, 9);
    fail_unless(result == 0x1F4,
            "First 4 bits in 0x%X was 0x%X instead of 0x1F4", data[0], result);
}
END_TEST

Suite* bitfieldSuite(void) {
    Suite* s = suite_create("bitfield");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_one_byte);
    tcase_add_test(tc_core, test_multi_byte);
    tcase_add_test(tc_core, test_get_multi_byte);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = bitfieldSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
