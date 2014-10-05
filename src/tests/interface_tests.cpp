#include <check.h>
#include <stdint.h>

#include "interface/interface.h"

namespace interface = openxc::interface;

using openxc::interface::InterfaceDescriptor;
using openxc::interface::InterfaceType;

START_TEST (test_usb_descriptor_string)
{
    InterfaceDescriptor descriptor;
    descriptor.type = interface::InterfaceType::USB;
    ck_assert_str_eq("USB", interface::descriptorToString(&descriptor));
}
END_TEST

START_TEST (test_uart_descriptor_string)
{
    InterfaceDescriptor descriptor;
    descriptor.type = interface::InterfaceType::UART;
    ck_assert_str_eq("UART", interface::descriptorToString(&descriptor));
}
END_TEST

START_TEST (test_network_descriptor_string)
{
    InterfaceDescriptor descriptor;
    descriptor.type = interface::InterfaceType::NETWORK;
    ck_assert_str_eq("NET", interface::descriptorToString(&descriptor));
}
END_TEST

START_TEST (test_unknown_descriptor_string)
{
    InterfaceDescriptor descriptor;
    descriptor.type = (InterfaceType) 5;
    ck_assert_str_eq("Unknown", interface::descriptorToString(&descriptor));
}
END_TEST

Suite* buffersSuite(void) {
    Suite* s = suite_create("interface");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_usb_descriptor_string);
    tcase_add_test(tc_core, test_uart_descriptor_string);
    tcase_add_test(tc_core, test_network_descriptor_string);
    tcase_add_test(tc_core, test_unknown_descriptor_string);
    suite_add_tcase(s, tc_core);
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
