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
using openxc::signals::handlers::sendTirePressure;
using openxc::signals::handlers::sendDoorStatus;
using openxc::signals::handlers::handleOccupancyMessage;
using openxc::signals::handlers::handleFuelFlow;
using openxc::signals::handlers::handleInverted;
using openxc::pipeline::Pipeline;
using openxc::signals::getSignalCount;
using openxc::signals::getSignals;
using openxc::signals::getCanBuses;
using openxc::config::getConfiguration;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[IN_ENDPOINT_INDEX].queue;

bool queueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

void setup() {
    openxc::config::getConfiguration()->messageSetIndex = 1;
    usb::initialize(&getConfiguration()->usb);
    getConfiguration()->usb.configured = true;
    for(int i = 0; i < getSignalCount(); i++) {
        getSignals()[i].received = false;
        getSignals()[i].frequencyClock = {0};
    }
}

START_TEST (test_inverted_handler)
{
    bool send = true;
    float result = handleInverted(&getSignals()[0], getSignals(), getSignalCount(),
            &getConfiguration()->pipeline, 1, &send);
    ck_assert(result == -1.0);
}
END_TEST

START_TEST (test_button_event_handler)
{
    fail_unless(queueEmpty());
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[0], encodeState(&getSignals()[0], "down", &send), data, sizeof(data));
    buildMessage(&getSignals()[1], encodeState(&getSignals()[1], "stuck", &send), data, sizeof(data));
    handleButtonEventMessage(0, data, getSignals(), getSignalCount(),
            &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_button_event_handler_bad_type)
{
    fail_unless(queueEmpty());
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[0], encodeState(&getSignals()[0], "bad", &send), data, sizeof(data));
    buildMessage(&getSignals()[1], encodeState(&getSignals()[1], "stuck", &send), data, sizeof(data));
    handleButtonEventMessage(0, data, getSignals(), getSignalCount(),
            &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_button_event_handler_correct_types)
{
    fail_unless(queueEmpty());
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[0], encodeState(&getSignals()[0], "down", &send), data, sizeof(data));
    buildMessage(&getSignals()[1], encodeState(&getSignals()[1], "stuck", &send), data, sizeof(data));
    handleButtonEventMessage(0, data, getSignals(), getSignalCount(),
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
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[0], encodeState(&getSignals()[0], "down", &send), data, sizeof(data));
    buildMessage(&getSignals()[1], encodeNumber(&getSignals()[1], 11, &send), data, sizeof(data));
    handleButtonEventMessage(0, data, getSignals(), getSignalCount(),
            &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_tire_pressure_handler)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[7], encodeNumber(&getSignals()[7], 23.1, &send), data, sizeof(data));
    sendTirePressure("foo", data, 1, &getSignals()[7], getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "foo") == NULL);
}
END_TEST

START_TEST (test_send_invalid_tire)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[7], encodeNumber(&getSignals()[7], true, &send), data, sizeof(data));
    sendDoorStatus("does-not-exist", data, NULL, getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_send_same_tire_pressure)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[7], encodeNumber(&getSignals()[7], true, &send), data, sizeof(data));
    sendDoorStatus("front_left", data, &getSignals()[7], getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    sendDoorStatus("front_left", data, &getSignals()[7], getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_occupancy_handler_child)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[11], encodeBoolean(&getSignals()[11], true, &send), data, sizeof(data));
    buildMessage(&getSignals()[12], encodeBoolean(&getSignals()[12], false, &send), data, sizeof(data));
    handleOccupancyMessage(getSignals()[11].message->id, data,
            getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "passenger") == NULL);
    fail_if(strstr((char*)snapshot, "child") == NULL);
    fail_unless(strstr((char*)snapshot, "adult") == NULL);
    fail_unless(strstr((char*)snapshot, "empty") == NULL);
}
END_TEST

START_TEST (test_occupancy_handler_adult)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[11], encodeNumber(&getSignals()[11], true, &send), data, sizeof(data));
    buildMessage(&getSignals()[12], encodeNumber(&getSignals()[12], true, &send), data, sizeof(data));
    handleOccupancyMessage(getSignals()[11].message->id, data,
            getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "passenger") == NULL);
    fail_if(strstr((char*)snapshot, "adult") == NULL);
    fail_unless(strstr((char*)snapshot, "child") == NULL);
    fail_unless(strstr((char*)snapshot, "empty") == NULL);
}
END_TEST

START_TEST (test_occupancy_handler_empty)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[11], encodeBoolean(&getSignals()[11], false, &send), data, sizeof(data));
    buildMessage(&getSignals()[12], encodeBoolean(&getSignals()[12], false, &send), data, sizeof(data));
    handleOccupancyMessage(getSignals()[11].message->id, data,
            getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "passenger") == NULL);
    fail_if(strstr((char*)snapshot, "empty") == NULL);
    fail_unless(strstr((char*)snapshot, "child") == NULL);
    fail_unless(strstr((char*)snapshot, "adult") == NULL);
}
END_TEST

START_TEST (test_fuel_handler)
{
    bool send = true;
    float result = 0;
    CanSignal* signal = &getSignals()[6];
    result = handleFuelFlow(signal, getSignals(), getSignalCount(), 0, &send, 1);
    ck_assert_int_eq(0, result);
    signal->lastValue = 0;

    result = handleFuelFlow(signal, getSignals(), getSignalCount(), 1, &send, 1);
    ck_assert_int_eq(1, result);
    signal->lastValue = 1;

    result = handleFuelFlow(signal, getSignals(), getSignalCount(), 255, &send, 1);
    ck_assert_int_eq(255, result);
    signal->lastValue = 255;

    result = handleFuelFlow(signal, getSignals(), getSignalCount(), 2, &send, 1);
    ck_assert_int_eq(257, result);
    signal->lastValue = 2;
}
END_TEST

START_TEST (test_door_handler)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[2], encodeNumber(&getSignals()[2], true, &send), data, sizeof(data));
    sendDoorStatus("foo", data, &getSignals()[2], getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    fail_if(strstr((char*)snapshot, "foo") == NULL);
}
END_TEST

START_TEST (test_send_invalid_door_status)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[2], encodeNumber(&getSignals()[2], true, &send), data, sizeof(data));
    sendDoorStatus("does-not-exist", data, NULL, getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_send_same_door_status)
{
    bool send = true;
    uint8_t data[8] = {0};
    buildMessage(&getSignals()[2], encodeNumber(&getSignals()[2], true, &send), data, sizeof(data));
    sendDoorStatus("driver", data, &getSignals()[2], getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    sendDoorStatus("driver", data, &getSignals()[2], getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
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
    tcase_add_test(tc_door_handler, test_send_invalid_door_status);
    tcase_add_test(tc_door_handler, test_send_same_door_status);
    suite_add_tcase(s, tc_door_handler);

    TCase *tc_fuel_handler = tcase_create("fuel");
    tcase_add_checked_fixture(tc_fuel_handler, setup, NULL);
    tcase_add_test(tc_fuel_handler, test_fuel_handler);
    suite_add_tcase(s, tc_fuel_handler);

    TCase *tc_tire_pressure = tcase_create("tire_pressure");
    tcase_add_checked_fixture(tc_tire_pressure, setup, NULL);
    tcase_add_test(tc_tire_pressure, test_tire_pressure_handler);
    tcase_add_test(tc_tire_pressure, test_send_invalid_tire);
    tcase_add_test(tc_tire_pressure, test_send_same_tire_pressure);
    suite_add_tcase(s, tc_tire_pressure);

    TCase *tc_occupancy = tcase_create("occupancy");
    tcase_add_checked_fixture(tc_occupancy, setup, NULL);
    tcase_add_test(tc_occupancy, test_occupancy_handler_child);
    tcase_add_test(tc_occupancy, test_occupancy_handler_adult);
    tcase_add_test(tc_occupancy, test_occupancy_handler_empty);
    suite_add_tcase(s, tc_occupancy);

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
