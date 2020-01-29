#include <check.h>
#include <stdint.h>
#include "signals.h"
#include "can/canread.h"
#include "can/canwrite.h"
#include "config.h"

#include "canutil_spy.h"

namespace can = openxc::can;

using openxc::can::lookupSignal;
using openxc::can::lookupSignalState;
using openxc::can::lookupMessageDefinition;
using openxc::can::registerMessageDefinition;
using openxc::can::unregisterMessageDefinition;
using openxc::can::setAcceptanceFilterStatus;
using openxc::signals::getCanBusCount;
using openxc::signals::getCanBuses;
using openxc::signals::getMessages;
using openxc::signals::getMessageCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalCount;
using openxc::signals::getSignalManagers;
using openxc::can::lookupSignalManagerDetails;
using openxc::signals::getCommands;
using openxc::signals::getCommandCount;

void setup() {
    for(int i = 0; i < getCanBusCount(); i++) {
        can::initializeCommon(&getCanBuses()[i]);
    }
}

void teardown() {
    for(int i = 0; i < getCanBusCount(); i++) {
        can::destroy(&getCanBuses()[i]);
    }
}

uint32_t MESSAGE_ID = 42;

START_TEST (test_can_signal_struct)
{
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    ck_assert_int_eq(testSignal->message->id, 0);
    ck_assert_str_eq(testSignal->genericName, "torque_at_transmission");
    ck_assert_int_eq(testSignal->bitPosition, 2);
    ck_assert_int_eq(testSignal->bitSize, 4);
    ck_assert_int_eq(testSignal->factor, 1001.0);
    ck_assert_int_eq(testSignal->offset, -30000.0);
    ck_assert_int_eq(testSignal->minValue, -5000.0);
    ck_assert_int_eq(testSignal->maxValue, 33522.0);

    testSignal = &getSignals()[1];
    signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    ck_assert_int_eq(signalManager->lastValue, 4.0);
}
END_TEST

START_TEST (test_can_signal_states)
{
    const CanSignal* testSignal = &getSignals()[1];
    ck_assert_int_eq(testSignal->message->id, 1);
    ck_assert_int_eq(testSignal->stateCount, 6);
    ck_assert_int_eq(testSignal->states[0].value, 1);
    ck_assert_str_eq(testSignal->states[0].name, "reverse");
}
END_TEST

START_TEST (test_lookup_signal)
{
    fail_unless(lookupSignal("does_not_exist", getSignals(), getSignalCount()) == NULL);
    fail_unless(lookupSignal("torque_at_transmission", getSignals(), getSignalCount())
            == &getSignals()[0]);
    fail_unless(lookupSignal("transmission_gear_position", getSignals(),
            getSignalCount()) == &getSignals()[1]);
}
END_TEST

START_TEST (test_lookup_writable_signal)
{
    fail_unless(lookupSignal("does_not_exist", getSignals(), getSignalCount(), true
                ) == NULL);
    fail_unless(lookupSignal("transmission_gear_position", getSignals(),
            getSignalCount(), false) == &getSignals()[1]);
    fail_unless(lookupSignal("command", getSignals(),
            getSignalCount(), false) == &getSignals()[4]);
    //fail_unless(lookupSignal("command", getSignals(),
    //        getSignalCount(), true) == &getSignals()[5]);
}
END_TEST

START_TEST (test_lookup_signal_state_by_name)
{
    fail_unless(lookupSignalState("does_not_exist", &getSignals()[1]) == NULL);
    fail_unless(lookupSignalState("reverse", &getSignals()[1]) == &getSignals()[1].states[0]);
    fail_unless(lookupSignalState("third", &getSignals()[1]) == &getSignals()[1].states[1]);
}
END_TEST

START_TEST (test_lookup_signal_state_by_value)
{
    fail_unless(lookupSignalState(MESSAGE_ID, &getSignals()[1]) == NULL);
    fail_unless(lookupSignalState(1, &getSignals()[1]) == &getSignals()[1].states[0]);
    fail_unless(lookupSignalState(2, &getSignals()[1]) == &getSignals()[1].states[1]);
}
END_TEST

START_TEST (test_lookup_command)
{
    fail_unless(lookupCommand("does_not_exist", getCommands(), getCommandCount()
                ) == NULL);
    fail_unless(lookupCommand("turn_signal_status", getCommands(), getCommandCount())
            == &getCommands()[0]);
}
END_TEST

START_TEST (test_initialize)
{
    CanBus bus = {500, 0x101};
    can::initializeCommon(&bus);
    can::destroy(&bus);
}
END_TEST

START_TEST (test_get_can_message_definition_predefined)
{
    CanMessageDefinition* message = lookupMessageDefinition(&getCanBuses()[0], 1,
            CanMessageFormat::STANDARD, getMessages(), getMessageCount());
    ck_assert(message == &getMessages()[1]);
}
END_TEST

START_TEST (test_get_can_message_definition_undefined)
{
    CanMessageDefinition* message = lookupMessageDefinition(&getCanBuses()[0], 999,
            CanMessageFormat::STANDARD, getMessages(), getMessageCount());
    ck_assert(message == NULL);
}
END_TEST

START_TEST (test_register_can_message)
{
    ck_assert(registerMessageDefinition(&getCanBuses()[0], MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount()));
    CanMessageDefinition* message = lookupMessageDefinition(&getCanBuses()[0],
            MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount());
    ck_assert(message != NULL);
    ck_assert_int_eq(message->id, MESSAGE_ID);
    ck_assert(message->bus == &getCanBuses()[0]);
}
END_TEST

START_TEST (test_register_can_message_twice)
{
    ck_assert(registerMessageDefinition(&getCanBuses()[0], MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount()));
    ck_assert(registerMessageDefinition(&getCanBuses()[0], MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount()));
    CanMessageDefinition* message = lookupMessageDefinition(&getCanBuses()[0],
            MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount());
    ck_assert(message != NULL);
    ck_assert_int_eq(message->id, MESSAGE_ID);
    ck_assert(message->bus == &getCanBuses()[0]);
}
END_TEST

START_TEST (test_register_can_message_diff_bus)
{
    ck_assert(registerMessageDefinition(&getCanBuses()[0], MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount()));
    ck_assert(registerMessageDefinition(&getCanBuses()[1], MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount()));
    CanMessageDefinition* message = lookupMessageDefinition(&getCanBuses()[0],
            MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount());
    ck_assert(message != NULL);
    ck_assert_int_eq(message->id, MESSAGE_ID);
    ck_assert(message->bus == &getCanBuses()[0]);

    message = lookupMessageDefinition(&getCanBuses()[1], MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(),
            getMessageCount());
    ck_assert(message != NULL);
    ck_assert_int_eq(message->id, MESSAGE_ID);
    ck_assert(message->bus == &getCanBuses()[1]);
}
END_TEST

START_TEST (test_unregister_can_message)
{
    ck_assert(registerMessageDefinition(&getCanBuses()[0], MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount()));
    ck_assert(lookupMessageDefinition(&getCanBuses()[0], MESSAGE_ID,
            CanMessageFormat::STANDARD, getMessages(), getMessageCount()) != NULL);
    ck_assert(unregisterMessageDefinition(&getCanBuses()[0], MESSAGE_ID, CanMessageFormat::STANDARD));
    ck_assert(lookupMessageDefinition(&getCanBuses()[0], MESSAGE_ID,
            CanMessageFormat::STANDARD, getMessages(), getMessageCount()) == NULL);
}
END_TEST

START_TEST (test_unregister_can_message_not_registered)
{
    ck_assert(registerMessageDefinition(&getCanBuses()[0], MESSAGE_ID, CanMessageFormat::STANDARD, getMessages(), getMessageCount()));
    ck_assert(lookupMessageDefinition(&getCanBuses()[0], MESSAGE_ID,
            CanMessageFormat::STANDARD, getMessages(), getMessageCount()) != NULL);
    ck_assert(unregisterMessageDefinition(&getCanBuses()[0], MESSAGE_ID, CanMessageFormat::STANDARD));
    ck_assert(lookupMessageDefinition(&getCanBuses()[0], MESSAGE_ID,
            CanMessageFormat::STANDARD, getMessages(), getMessageCount()) == NULL);
}
END_TEST

START_TEST (test_unregister_predefined)
{
    // it should have no effect
    ck_assert(!unregisterMessageDefinition(&getCanBuses()[0], 1, CanMessageFormat::STANDARD));
}
END_TEST

START_TEST (test_set_acceptance_filter_status)
{
    ck_assert(setAcceptanceFilterStatus(&getCanBuses()[0], true, getCanBuses(), getCanBusCount()));
    ck_assert(can::spy::acceptanceFiltersUpdated());
    ck_assert(!getCanBuses()[0].bypassFilters);

    ck_assert(setAcceptanceFilterStatus(&getCanBuses()[0], false, getCanBuses(), getCanBusCount()));
    ck_assert(can::spy::acceptanceFiltersUpdated());
    ck_assert(getCanBuses()[0].bypassFilters);
}
END_TEST

Suite* canutilSuite(void) {
    Suite* s = suite_create("canutil");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_initialize);
    tcase_add_test(tc_core, test_can_signal_struct);
    tcase_add_test(tc_core, test_can_signal_states);
    tcase_add_test(tc_core, test_lookup_signal);
    tcase_add_test(tc_core, test_lookup_writable_signal);
    tcase_add_test(tc_core, test_lookup_signal_state_by_name);
    tcase_add_test(tc_core, test_lookup_signal_state_by_value);
    tcase_add_test(tc_core, test_lookup_command);
    tcase_add_test(tc_core, test_set_acceptance_filter_status);
    suite_add_tcase(s, tc_core);

    TCase *tc_message_def = tcase_create("message_definitions");
    tcase_add_checked_fixture(tc_message_def, setup, teardown);
    tcase_add_test(tc_message_def, test_get_can_message_definition_predefined);
    tcase_add_test(tc_message_def, test_get_can_message_definition_undefined);
    tcase_add_test(tc_message_def, test_register_can_message);
    tcase_add_test(tc_message_def, test_register_can_message_twice);
    tcase_add_test(tc_message_def, test_register_can_message_diff_bus);
    tcase_add_test(tc_message_def, test_unregister_can_message);
    tcase_add_test(tc_message_def, test_unregister_can_message_not_registered);
    tcase_add_test(tc_message_def, test_unregister_predefined);
    suite_add_tcase(s, tc_message_def);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = canutilSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
