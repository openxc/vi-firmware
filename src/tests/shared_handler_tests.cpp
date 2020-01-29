#include <check.h>
#include <stdint.h>
#include "shared_handlers.h"
#include "can/canwrite.h"
#include "config.h"
#include "signals.h"

namespace usb = openxc::interface::usb;

using openxc::can::write::encodeState;
using openxc::can::write::encodeNumber;
using openxc::can::write::encodeBoolean;
using openxc::can::write::buildMessage;
using openxc::signals::handlers::handleButtonEventMessage;
using openxc::signals::handlers::tirePressureDecoder;
using openxc::signals::handlers::doorStatusDecoder;
using openxc::signals::handlers::handleFuelFlow;
using openxc::signals::handlers::handleInverted;
using openxc::pipeline::Pipeline;
using openxc::signals::getSignalCount;
using openxc::signals::getSignals;
using openxc::signals::getSignalManagers;
using openxc::can::lookupSignalManagerDetails;
using openxc::signals::getCanBuses;
using openxc::config::getConfiguration;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[IN_ENDPOINT_INDEX].queue;

bool queueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

void setup() {
    getConfiguration()->pipeline.uart = NULL;
    openxc::config::getConfiguration()->messageSetIndex = 1;
    usb::initialize(&getConfiguration()->usb);
    getConfiguration()->usb.configured = true;
    for(int i = 0; i < getSignalCount(); i++) {
        SignalManager* signalManager = lookupSignalManagerDetails(getSignals()[i].genericName, getSignalManagers(), getSignalCount());
        signalManager->received = false;
        signalManager->frequencyClock = {0};
    }
}

START_TEST (test_inverted_handler)
{
    bool send = true;
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    openxc_DynamicField result = handleInverted(testSignal, getSignals(), signalManager, getSignalManagers(), getSignalCount(),
            &getConfiguration()->pipeline, 1, &send);
    ck_assert(result.numeric_value == -1.0);
}
END_TEST

START_TEST (test_button_event_handler)
{
    fail_unless(queueEmpty());
    bool send = true;
    CanMessage message = {0};
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    buildMessage(&getSignals()[0], encodeState(&getSignals()[0], "down", &send), message.data, sizeof(message.data));
    buildMessage(&getSignals()[1], encodeState(&getSignals()[1], "stuck", &send), message.data, sizeof(message.data));
    handleButtonEventMessage(&getSignals()[0], getSignals(), signalManager, getSignalManagers(), getSignalCount(), &message,
            &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_button_event_handler_bad_type)
{
    fail_unless(queueEmpty());
    bool send = true;
    CanMessage message = {0};
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    buildMessage(&getSignals()[0], encodeState(&getSignals()[0], "bad", &send), message.data, sizeof(message.data));
    buildMessage(&getSignals()[1], encodeState(&getSignals()[1], "stuck", &send), message.data, sizeof(message.data));
    handleButtonEventMessage(&getSignals()[0], getSignals(), signalManager, getSignalManagers(), getSignalCount(), &message,
            &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_button_event_handler_correct_types)
{
    fail_unless(queueEmpty());
    bool send = true;
    CanMessage message = {0};
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    buildMessage(&getSignals()[0], encodeState(&getSignals()[0], "down", &send), message.data, sizeof(message.data));
    buildMessage(&getSignals()[1], encodeState(&getSignals()[1], "stuck", &send), message.data, sizeof(message.data));
    handleButtonEventMessage(&getSignals()[0], getSignals(), signalManager, getSignalManagers(), getSignalCount(), &message,
            &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "event") == NULL);
    fail_if(strstr((char*)snapshot, "value") == NULL);
    fail_if(strstr((char*)snapshot, "stuck") == NULL);
    fail_if(strstr((char*)snapshot, "down") == NULL);
}
END_TEST

START_TEST (test_button_event_handler_bad_state)
{
    fail_unless(queueEmpty());
    bool send = true;
    CanMessage message = {0};
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    buildMessage(&getSignals()[0], encodeState(&getSignals()[0], "down", &send), message.data, sizeof(message.data));
    buildMessage(&getSignals()[1], encodeNumber(&getSignals()[1], 11, &send), message.data, sizeof(message.data));
    handleButtonEventMessage(&getSignals()[0], getSignals(), signalManager, getSignalManagers(), getSignalCount(), &message,
            &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_tire_pressure_as_decoder)
{
    bool send = true;
    const CanSignal* signal = &getSignals()[7];
    ((CanSignal*)signal)->decoder = &tirePressureDecoder;
    CanMessage message = {
        id: signal->message->id,
        format: CanMessageFormat::STANDARD,
        data: {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
        length: 8
    };
    SignalManager* signalManager = lookupSignalManagerDetails(signal->genericName, getSignalManagers(), getSignalCount());
    openxc::can::read::decodeSignal(signal, getSignals(), signalManager,
        getSignalManagers(), getSignalCount(), &message, &send);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "front_left") == NULL);
}
END_TEST

START_TEST (test_tire_pressure_handler)
{
    bool send = true;
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    openxc_DynamicField decodedTireId = tirePressureDecoder(&getSignals()[7],
            getSignals(), signalManager, getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline,
            23.1, &send);
    // This decoder handles sending its own messages
    fail_if(queueEmpty());
    ck_assert_str_eq(decodedTireId.string_value, "front_left");

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "front_left") == NULL);
}
END_TEST

START_TEST (test_fuel_handler)
{
    bool send = true;
    float result = 0;
    const CanSignal* signal = &getSignals()[6];
    SignalManager* signalManager = lookupSignalManagerDetails(signal->genericName, getSignalManagers(), getSignalCount());
    result = handleFuelFlow(signal, getSignals(), signalManager, getSignalManagers(), getSignalCount(), 0, &send, 1).numeric_value;
    ck_assert_int_eq(0, result);
    signalManager->lastValue = 0;

    result = handleFuelFlow(signal, getSignals(), signalManager, getSignalManagers(), getSignalCount(), 1, &send, 1).numeric_value;
    ck_assert_int_eq(1, result);
    signalManager->lastValue = 1;

    result = handleFuelFlow(signal, getSignals(), signalManager, getSignalManagers(), getSignalCount(), 255, &send, 1).numeric_value;
    ck_assert_int_eq(255, result);
    signalManager->lastValue = 255;

    result = handleFuelFlow(signal, getSignals(), signalManager, getSignalManagers(), getSignalCount(), 2, &send, 1).numeric_value;
    ck_assert_int_eq(257, result);
    signalManager->lastValue = 2;
}
END_TEST

START_TEST (test_door_handler)
{
    bool send = true;
    const CanSignal* testSignal = &getSignals()[0];
    SignalManager* signalManager = lookupSignalManagerDetails(testSignal->genericName, getSignalManagers(), getSignalCount());
    openxc_DynamicField decodedDoorId = doorStatusDecoder(&getSignals()[2],
            getSignals(), signalManager, getSignalManagers(), getSignalCount(), &getConfiguration()->pipeline, 1,
            &send);
    // This decoder handles sending its own messages
    fail_if(queueEmpty());
    ck_assert_str_eq(decodedDoorId.string_value, "driver");

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "driver") == NULL);
}
END_TEST

Suite* handlerSuite(void) {
    Suite* s = suite_create("shared_handlers");

    TCase *tc_number_handlers = tcase_create("numbers");
    tcase_add_checked_fixture(tc_number_handlers, setup, NULL);
    tcase_add_test(tc_number_handlers, test_inverted_handler);
    suite_add_tcase(s, tc_number_handlers);

    TCase *tc_button_handler = tcase_create("button");
    tcase_add_checked_fixture(tc_button_handler, setup, NULL);
    tcase_add_test(tc_button_handler, test_button_event_handler);
    tcase_add_test(tc_button_handler, test_button_event_handler_bad_type);
    tcase_add_test(tc_button_handler, test_button_event_handler_bad_state);
    tcase_add_test(tc_button_handler, test_button_event_handler_correct_types);
    suite_add_tcase(s, tc_button_handler);

    TCase *tc_door_handler = tcase_create("door");
    tcase_add_checked_fixture(tc_door_handler, setup, NULL);
    tcase_add_test(tc_door_handler, test_door_handler);
    suite_add_tcase(s, tc_door_handler);

    TCase *tc_fuel_handler = tcase_create("fuel");
    tcase_add_checked_fixture(tc_fuel_handler, setup, NULL);
    tcase_add_test(tc_fuel_handler, test_fuel_handler);
    suite_add_tcase(s, tc_fuel_handler);

    TCase *tc_tire_pressure = tcase_create("tire_pressure");
    tcase_add_checked_fixture(tc_tire_pressure, setup, NULL);
    tcase_add_test(tc_tire_pressure, test_tire_pressure_handler);
    tcase_add_test(tc_tire_pressure, test_tire_pressure_as_decoder);
    suite_add_tcase(s, tc_tire_pressure);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = handlerSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
