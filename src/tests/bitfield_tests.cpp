#include <check.h>
#include <stdint.h>
#include "bitfield.h"

// uint64_t is stored in little endian but we read it in big endian, so the
// retrieval in the set tests may look a little funky
uint64_t swappingGetBitField(uint64_t data, int startBit, int numBits) {
    return getBitField(__builtin_bswap64(data), startBit, numBits);
}

START_TEST (test_one_bit_not_swapped)
{
    uint64_t data = 0x80;
    uint64_t result = swappingGetBitField(data, 0, 1);
    fail_if(result == 1);
}
END_TEST

START_TEST (test_one_bit)
{
    uint64_t data = 0x8000000000000000;
    uint64_t result = swappingGetBitField(data, 0, 1);
    fail_unless(result == 0x1,
            "First bits in 0x%X was 0x%X instead of 0x1", data, result);
}
END_TEST

START_TEST (test_full_message)
{
    uint64_t data = 0xF34DFCFF00000000;
    uint64_t result = swappingGetBitField(data, 16, 16);
    uint64_t expectedValue = 0xFCFF;
    fail_unless(result == expectedValue,
            "Field retrieved in 0x%X was 0x%X instead of %d", data,
            result, expectedValue);
}
END_TEST

START_TEST (test_one_byte)
{
    uint64_t data = 0xFA00000000000000;
    uint64_t result = swappingGetBitField(data, 0, 4);
    fail_unless(result == 0xF,
            "First 4 bits in 0x%X was 0x%X instead of 0xF", data, result);
    result = swappingGetBitField(data, 4, 4);
    fail_unless(result == 0xA,
            "First 4 bits in 0x%X was 0x%X instead of 0xA", data, result);
    result = swappingGetBitField(data, 0, 8);
    fail_unless(result == 0xFA,
            "All bits in 0x%X were 0x%X instead of 0x%X", data, result, data);
}
END_TEST

START_TEST (test_multi_byte)
{
    uint64_t data = 0x12FA000000000000;
    uint64_t result = swappingGetBitField(data, 0, 4);
    fail_unless(result == 0x1,
            "First 4 bits in 0x%X was 0x%X instead of 0xF", (data >> 60) & 0xF,
            result);
    result = swappingGetBitField(data, 4, 4);
    fail_unless(result == 0x2,
            "Second 4 bits in 0x%X was %d instead of 0xA", (data >> 56) & 0xF,
            result);
    result = swappingGetBitField(data, 8, 4);
    fail_unless(result == 0xF,
            "First 4 bits in 0x%X was %d instead of 0x1", (data >> 52) & 0xF,
            result);
    result = swappingGetBitField(data, 12, 4);
    fail_unless(result == 0xA,
            "Second 4 bits in 0x%X was %d instead of 0x2", (data >> 48) % 0xF,
            result);
}
END_TEST

START_TEST (test_get_multi_byte)
{
    uint64_t data = 0x12FA000000000000;
    uint64_t result = swappingGetBitField(data, 0, 9);
    ck_assert_int_eq(result, 0x25);
}
END_TEST

START_TEST (test_get_off_byte_boundary)
{
    uint64_t data = 0x000012FA00000000;
    uint64_t result = swappingGetBitField(data, 12, 8);
    ck_assert_int_eq(result, 0x01);
} END_TEST

START_TEST (test_set_field)
{
    uint64_t data = 0;
    setBitField(&data, 1, 0, 1);
    uint64_t result = swappingGetBitField(data, 0, 1);
    ck_assert_int_eq(result, 0x1);
    data = 0;
    setBitField(&data, 1, 1, 1);
    result = swappingGetBitField(data, 1, 1);
    ck_assert_int_eq(result, 0x1);

    data = 0;
    setBitField(&data, 0xf, 3, 4);
    result = swappingGetBitField(data, 3, 4);
    ck_assert_int_eq(result, 0xf);
}
END_TEST

START_TEST (test_set_doesnt_clobber_existing_data)
{
    uint64_t data = 0xFFFC4DF300000000;
    setBitField(&data, 0x4fc8, 16, 16);
    uint64_t result = swappingGetBitField(data, 16, 16);
    fail_unless(result == 0x4fc8,
            "Field retrieved in 0x%X was 0x%X instead of 0x%X", data, result,
            0xc84f);

    data = 0x8000000000000000;
    setBitField(&data, 1, 21, 1);
    fail_unless(data == 0x8000040000000000LLU,
            "Expected combined value 0x8000040000000000 but got 0x%X%X",
            data >> 32, data);
}
END_TEST

START_TEST (test_set_off_byte_boundary)
{
    uint64_t data = 0xFFFC4DF300000000;
    setBitField(&data, 0x12, 12, 8);
    uint64_t result = swappingGetBitField(data, 12, 12);
    ck_assert_int_eq(result,0x12d);
}
END_TEST

START_TEST (test_set_odd_number_of_bits)
{
    uint64_t data = 0xFFFC4DF300000000LLU;
    setBitField(&data, 0x12, 11, 5);
    uint64_t result = swappingGetBitField(data, 11, 5);
    fail_unless(result == 0x12,
            "Field set in 0x%X%X%X%X was %d instead of %d", data, result,
            0x12);

    data = 0xFFFC4DF300000000LLU;
    setBitField(&data, 0x2, 11, 5);
    result = swappingGetBitField(data, 11, 5);
    fail_unless(result == 0x2,
            "Field set in 0x%X%X%X%X was %d instead of %d", data, result,
            0x2);
}
END_TEST

START_TEST(test_nth_byte)
{
    uint64_t data = 0x00000000F34DFCFF;
    uint8_t result = nthByte(data, 0);
    uint8_t expected = 0x0;
    ck_assert_int_eq(result, expected);

    result = nthByte(data, 4);
    expected = 0xF3;
    ck_assert_int_eq(result, expected);

    result = nthByte(data, 5);
    expected = 0x4D;
    ck_assert_int_eq(result, expected);

    result = nthByte(data, 6);
    expected = 0xFC;
    ck_assert_int_eq(result, expected);

    result = nthByte(data, 7);
    expected = 0xFF;
    ck_assert_int_eq(result, expected);
}
END_TEST

Suite* bitfieldSuite(void) {
    Suite* s = suite_create("bitfield");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_one_bit);
    tcase_add_test(tc_core, test_one_bit_not_swapped);
    tcase_add_test(tc_core, test_one_byte);
    tcase_add_test(tc_core, test_full_message);
    tcase_add_test(tc_core, test_multi_byte);
    tcase_add_test(tc_core, test_get_multi_byte);
    tcase_add_test(tc_core, test_get_off_byte_boundary);
    tcase_add_test(tc_core, test_set_field);
    tcase_add_test(tc_core, test_set_doesnt_clobber_existing_data);
    tcase_add_test(tc_core, test_set_off_byte_boundary);
    tcase_add_test(tc_core, test_set_odd_number_of_bits);
    tcase_add_test(tc_core, test_nth_byte);
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
