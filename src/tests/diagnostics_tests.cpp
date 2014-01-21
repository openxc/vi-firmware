#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "diagnostics.h"

namespace diagnostics = openxc::diagnostics;

using openxc::signals::getCanBuses;
using openxc::signals::getCanBusCount;
using openxc::signals::getMessages;
using openxc::pipeline::Pipeline;

diagnostics::DiagnosticsManager DIAGNOSTICS_MANAGER;
extern Pipeline PIPELINE;

void setup() {
    diagnostics::initialize(&DIAGNOSTICS_MANAGER, getCanBuses(), getCanBusCount());
}

START_TEST (test_add_basic_request)
{
    DiagnosticRequest request = {
        arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
        mode: OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST,
        pid: 0xc,
        pid_length: 1
    };
    diagnostics::addDiagnosticRequest(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &request, "foobar", NULL, 1);
    diagnostics::sendRequests(&DIAGNOSTICS_MANAGER, &getCanBuses()[0]);
    CanMessage message = {getMessages()[2].id, 0x1234};
    diagnostics::receiveCanMessage(&DIAGNOSTICS_MANAGER, &getCanBuses()[0],
            &message, &PIPELINE);
    fail_if(true);
}
END_TEST

START_TEST (test_add_request_with_name)
{
    fail_if(true);
}
END_TEST

START_TEST (test_add_request_with_decoder)
{
    fail_if(true);
}
END_TEST

START_TEST (test_add_request_with_name_and_decoder)
{
    fail_if(true);
}
END_TEST

START_TEST (test_add_request_with_frequency)
{
    fail_if(true);
}
END_TEST

START_TEST (test_add_request_with_everything)
{
    fail_if(true);
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("diagnostics");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_add_basic_request);
    tcase_add_test(tc_core, test_add_request_with_name);
    tcase_add_test(tc_core, test_add_request_with_decoder);
    tcase_add_test(tc_core, test_add_request_with_name_and_decoder);
    tcase_add_test(tc_core, test_add_request_with_frequency);
    tcase_add_test(tc_core, test_add_request_with_everything);

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
