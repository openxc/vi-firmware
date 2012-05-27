#include <check.h>
#include <stdint.h>
#include "bitfield.h"

START_TEST (test_one_bit)
{
    uint8_t data = 0x80;
    unsigned long result = getBitField(&data, 0, 1);
    fail_unless(result == 0x1,
            "First bits in 0x%X was 0x%X instead of 0x1", data, result);
}
END_TEST

START_TEST (test_full_message)
{
    uint8_t data[4] = {0xFF, 0xFC, 0x4D, 0xF3};
    unsigned long result = getBitField(data, 16, 16);
    unsigned long expectedValue = 19955;
    fail_unless(result == expectedValue,
            "Field retrieved in 0x%X%X%X%X was %d instead of %d", data[0],
            data[1], data[2], data[3], result, expectedValue);
}
END_TEST

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

START_TEST (test_get_off_byte_boundary)
{
    uint8_t data[4] = {0xFA, 0x12, 0x00, 0x00};
    unsigned long result = getBitField(data, 4, 8);
    fail_unless(result == 0xA1,
            "Middle 4 bits in 0x%X was %d instead of 0xA1", data, result);
} END_TEST

START_TEST (test_set_field)
{
    uint32_t data = 0;
    setBitField(&data, 1, 0, 1);
    // unit32_t is stored in little endian but we read it in big endian, so the
    // retrieval in the set tests may look a little funky
    unsigned long result = getBitField((uint8_t*)&data, 24, 1);
    fail_unless(result == 0x1);

    memset(&data, 0, 8);
    setBitField(&data, 1, 1, 1);
    result = getBitField((uint8_t*)&data, 25, 1);
    fail_unless(result == 0x1);

    memset(&data, 0, 8);
    setBitField(&data, 0xf, 3, 4);
    result = getBitField((uint8_t*)&data, 27, 4);
    fail_unless(result == 0xf);
}
END_TEST

START_TEST (test_set_doesnt_clobber_existing_data)
{
    uint32_t data = 0xFFFC4DF3;
    unsigned long expectedValue = 0x4fc8;
    setBitField(&data, expectedValue, 16, 16);
    // unit32_t is stored in little endian but we read it in big endian, so the
    // retrieval in the set tests may look a little funky
    unsigned long result = getBitField((uint8_t*)&data, 0, 16);
    fail_unless(result == 0xc84f,
            "Field retrieved in 0x%X was 0x%X instead of 0x%X", data, result,
            expectedValue);
}
END_TEST

START_TEST (test_set_off_byte_boundary)
{
    uint32_t data = 0xFFFC4DF3;
    unsigned long expectedValue = 0x12;
    setBitField(&data, expectedValue, 12, 8);
    unsigned long result = getBitField((uint8_t*)&data, 8, 16);
    fail_unless(result == 0x2df1,
            "Field set in 0x%X%X%X%X was %d instead of %d", data, result,
            expectedValue);
}
END_TEST

// TODO try getting and writing an odd number of bits

Suite* bitfieldSuite(void) {
    Suite* s = suite_create("bitfield");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_one_bit);
    tcase_add_test(tc_core, test_one_byte);
    tcase_add_test(tc_core, test_full_message);
    tcase_add_test(tc_core, test_multi_byte);
    tcase_add_test(tc_core, test_get_multi_byte);
    tcase_add_test(tc_core, test_get_off_byte_boundary);
    tcase_add_test(tc_core, test_set_field);
    tcase_add_test(tc_core, test_set_doesnt_clobber_existing_data);
    tcase_add_test(tc_core, test_set_off_byte_boundary);
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
