#include <check.h>
#include <stdint.h>
#include "bitfield.h"

START_TEST (test_one_bit)
{
    uint64_t data = 0x80;
    uint64_t result = getBitField(data, 0, 1);
    fail_unless(result == 0x1,
            "First bits in 0x%X was 0x%X instead of 0x1", data, result);
}
END_TEST

START_TEST (test_full_message)
{
    uint64_t data = 0xF34DFCFF;
    uint64_t result = getBitField(data, 16, 16);
    uint64_t expectedValue = 19955;
    fail_unless(result == expectedValue,
            "Field retrieved in 0x%X was %d instead of %d", data,
			result, expectedValue);
}
END_TEST

START_TEST (test_one_byte)
{
    uint64_t data = 0xFA;
    uint64_t result = getBitField(data, 0, 4);
    fail_unless(result == 0xF,
            "First 4 bits in 0x%X was 0x%X instead of 0xF", data, result);
    result = getBitField(data, 4, 4);
    fail_unless(result == 0xA,
            "First 4 bits in 0x%X was 0x%X instead of 0xA", data, result);
    result = getBitField(data, 0, 8);
    fail_unless(result == 0xFA,
            "All bits in 0x%X were 0x%X instead of 0x%X", data, result, data);
}
END_TEST

START_TEST (test_multi_byte)
{
    uint64_t data = 0x12FA;
    uint64_t result = getBitField(data, 0, 4);
    fail_unless(result == 0xF,
            "First 4 bits in 0x%X was %d instead of 0xF", (data >> 60) & 0xF,
			result);
    result = getBitField(data, 4, 4);
    fail_unless(result == 0xA,
            "Second 4 bits in 0x%X was %d instead of 0xA", (data >> 56) & 0xF,
			result);
    result = getBitField(data, 8, 4);
    fail_unless(result == 0x1,
            "First 4 bits in 0x%X was %d instead of 0x1", (data >> 52) & 0xF,
			result);
    result = getBitField(data, 12, 4);
    fail_unless(result == 0x2,
            "Second 4 bits in 0x%X was %d instead of 0x2", (data >> 48) % 0xF,
			result);
}
END_TEST

START_TEST (test_get_multi_byte)
{
    uint64_t data = 0x12FA;
    uint64_t result = getBitField(data, 0, 9);
    fail_unless(result == 0x1F4,
            "First 4 bits in 0x%X was 0x%X instead of 0x1F4",
			(data >> 60) & 0xF, result);
}
END_TEST

START_TEST (test_get_off_byte_boundary)
{
    uint64_t data = 0x000012FA;
    uint64_t result = getBitField(data, 4, 8);
    fail_unless(result == 0xA1,
            "Middle 4 bits in 0x%X was %d instead of 0xA1", data, result);
} END_TEST

START_TEST (test_set_field)
{
    uint64_t data = 0;
    setBitField(&data, 1, 0, 1);
    // unit32_t is stored in little endian but we read it in big endian, so the
    // retrieval in the set tests may look a little funky
    uint64_t result = getBitField(data, 56, 1);
    fail_unless(result == 0x1);

	data = 0;
    setBitField(&data, 1, 1, 1);
    result = getBitField(data, 57, 1);
    fail_unless(result == 0x1);

    data = 0;
    setBitField(&data, 0xf, 3, 4);
    result = getBitField(data, 59, 4);
    fail_unless(result == 0xf);
}
END_TEST

START_TEST (test_set_doesnt_clobber_existing_data)
{
    uint64_t data = 0xFFFC4DF3;
    setBitField(&data, 0x4fc8, 16, 16);
    // unit32_t is stored in little endian but we read it in big endian, so the
    // retrieval in the set tests may look a little funky
    uint64_t result = getBitField(data, 32, 16);
    fail_unless(result == 0xc84f,
            "Field retrieved in 0x%X was 0x%X instead of 0x%X", data, result,
            0xc84f);

    data = 0x8000000000000000LLU;
    setBitField(&data, 1, 21, 1);
    fail_unless(data == 0x8000040000000000LLU,
            "Expected combined value 0x8000040000000000 but got 0x%X%X",
            data >> 32, data);
}
END_TEST

START_TEST (test_set_off_byte_boundary)
{
    uint64_t data = 0xFFFC4DF300000000LLU;
    setBitField(&data, 0x12, 12, 8);
    uint64_t result = getBitField(data, 40, 16);
    fail_unless(result == 0x2df1,
            "Field set in 0x%X%X%X%X was %d instead of %d", data, result,
            0x2df1);
}
END_TEST

START_TEST (test_set_odd_number_of_bits)
{
    uint64_t data = 0xFFFC4DF300000000LLU;
    setBitField(&data, 0x12, 11, 5);
    uint64_t result = getBitField(data, 51, 5);
    fail_unless(result == 0x12,
            "Field set in 0x%X%X%X%X was %d instead of %d", data, result,
            0x12);

    data = 0xFFFC4DF300000000LLU;
    setBitField(&data, 0x2, 11, 5);
    result = getBitField(data, 51, 5);
    fail_unless(result == 0x2,
            "Field set in 0x%X%X%X%X was %d instead of %d", data, result,
            0x2);

    result = getBitField(data, 48, 4);
    fail_unless(result == 0xe,
            "Field set in 0x%X%X%X%X was %d instead of %d", data, result,
            0xe);
}
END_TEST

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
    tcase_add_test(tc_core, test_set_odd_number_of_bits);
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
