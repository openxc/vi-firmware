#include <check.h>
#include "bitfield.h"

START_TEST (test_foo)
{
    fail_unless(1 == 2);
}
END_TEST

Suite* bitfieldSuite(void) {
    Suite* s = suite_create("bitfield");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_foo);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = bitfieldSuite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
