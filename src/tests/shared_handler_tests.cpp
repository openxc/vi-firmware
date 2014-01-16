#include <check.h>
#include <stdint.h>
#include "shared_handlers.h"
#include "can/canwrite.h"

namespace usb = openxc::interface::usb;

using openxc::can::write::booleanWriter;
using openxc::can::write::stateWriter;
using openxc::can::write::numberWriter;
using openxc::signals::handlers::handleButtonEventMessage;
using openxc::signals::handlers::sendTirePressure;
using openxc::signals::handlers::sendDoorStatus;
using openxc::signals::handlers::handleOccupancyMessage;
using openxc::signals::handlers::handleFuelFlow;
using openxc::signals::handlers::handleInverted;

CanMessageDefinition MESSAGES[4] = {
    {NULL, 0},
    {NULL, 1},
    {NULL, 2},
    {NULL, 3},
};

const int SIGNAL_COUNT = 13;

CanSignalState SIGNAL_STATES[2][6] = {
    { {1, "right"}, {2, "down"}, {3, "left"}, {4, "ok"}, {5, "up"}, {6, "foo"}},
    { {1, "idle"}, {2, "stuck"}, {3, "held_short"}, {4, "pressed"},
        {5, "held_long"}, {6, "released"}, },
};

CanSignal SIGNALS[SIGNAL_COUNT] = {
    {&MESSAGES[0], "button_type", 8, 8, 1.000000, 0.000000, 0.000000,
        0.000000, {0}, true, false, SIGNAL_STATES[0], 5, false, NULL},
    {&MESSAGES[0], "button_state", 20, 4, 1.000000, 0.000000, 0.000000,
        0.000000, {0}, true, false, SIGNAL_STATES[1], 6, false, NULL},
    {&MESSAGES[1], "driver_door", 15, 1, 1.000000, 0.000000, 0.000000,
            0.000000},
    {&MESSAGES[1], "passenger_door", 16, 1, 1.000000, 0.000000, 0.000000,
            0.000000},
    {&MESSAGES[1], "rear_left_door", 17, 1, 1.000000, 0.000000, 0.000000,
            0.000000},
    {&MESSAGES[1], "rear_right_door", 18, 1, 1.000000, 0.000000, 0.000000,
            0.000000},
    {&MESSAGES[1], "fuel_consumed_since_restart", 18, 1, 25.000000, 0.000000,
            0.000000, 255.0},
    {&MESSAGES[2], "tire_pressure_front_left", 15, 1, 1.000000, 0.000000,
            0.000000, 0.000000},
    {&MESSAGES[2], "tire_pressure_front_right", 16, 1, 1.000000, 0.000000,
            0.000000, 0.000000},
    {&MESSAGES[2], "tire_pressure_rear_right", 17, 1, 1.000000, 0.000000,
            0.000000, 0.000000},
    {&MESSAGES[2], "tire_pressure_rear_left", 18, 1, 1.000000, 0.000000,
            0.000000, 0.000000},
    {&MESSAGES[3], "passenger_occupancy_lower", 17, 1, 1.000000, 0.000000,
            0.000000, 0.000000},
    {&MESSAGES[3], "passenger_occupancy_upper", 18, 1, 1.000000, 0.000000,
            0.000000, 0.000000},
};

extern Pipeline PIPELINE;
extern UsbDevice USB_DEVICE;

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &PIPELINE.usb->endpoints[IN_ENDPOINT_INDEX].queue;

bool queueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}

void setup() {
    PIPELINE.usb = &USB_DEVICE;
    usb::initialize(&USB_DEVICE);
    PIPELINE.usb->configured = true;
    for(int i = 0; i < SIGNAL_COUNT; i++) {
        SIGNALS[i].received = false;
        SIGNALS[i].frequencyClock = {0};
    }
}

START_TEST (test_inverted_handler)
{
    bool send = true;
    float result = handleInverted(&SIGNALS[0], SIGNALS, SIGNAL_COUNT,
            &PIPELINE, 1, &send);
    ck_assert(result == -1.0);
}
END_TEST

START_TEST (test_button_event_handler)
{
    fail_unless(queueEmpty());
    bool send = true;
    uint64_t data =  stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "down",
            &send);
    data += stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, "stuck", &send);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &PIPELINE);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_button_event_handler_bad_type)
{
    fail_unless(queueEmpty());
    bool send = true;
    uint64_t data =  stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "bad",
            &send);
    data += stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, "stuck", &send);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &PIPELINE);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_button_event_handler_correct_types)
{
    fail_unless(queueEmpty());
    bool send = true;
    uint64_t data =  stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "down",
            &send);
    data += stateWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, "stuck", &send);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &PIPELINE);
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
    uint64_t data = stateWriter(&SIGNALS[0], SIGNALS, SIGNAL_COUNT, "down",
            &send);
    data += numberWriter(&SIGNALS[1], SIGNALS, SIGNAL_COUNT, 11, &send);
    handleButtonEventMessage(0, __builtin_bswap64(data), SIGNALS, SIGNAL_COUNT,
            &PIPELINE);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_tire_pressure_handler)
{
    bool send = true;
    uint64_t data = numberWriter(&SIGNALS[7], SIGNALS, SIGNAL_COUNT, 23.1,
            &send);
    sendTirePressure("foo", __builtin_bswap64(data), 1, &SIGNALS[7], SIGNALS,
            SIGNAL_COUNT, &PIPELINE);
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
    uint64_t data = booleanWriter(&SIGNALS[7], SIGNALS, SIGNAL_COUNT, true,
            &send);
    sendDoorStatus("does-not-exist", __builtin_bswap64(data), NULL, SIGNALS,
            SIGNAL_COUNT, &PIPELINE);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_send_same_tire_pressure)
{
    bool send = true;
    uint64_t data = booleanWriter(&SIGNALS[7], SIGNALS, SIGNAL_COUNT, true,
            &send);
    sendDoorStatus("front_left", __builtin_bswap64(data), &SIGNALS[7], SIGNALS,
            SIGNAL_COUNT, &PIPELINE);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    sendDoorStatus("front_left", __builtin_bswap64(data), &SIGNALS[7], SIGNALS,
            SIGNAL_COUNT, &PIPELINE);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_occupancy_handler_child)
{
    bool send = true;
    uint64_t data = booleanWriter(&SIGNALS[11], SIGNALS, SIGNAL_COUNT, true,
            &send);
    data += booleanWriter(&SIGNALS[12], SIGNALS, SIGNAL_COUNT, false, &send);
    handleOccupancyMessage(SIGNALS[11].message->id, __builtin_bswap64(data),
            SIGNALS, SIGNAL_COUNT, &PIPELINE);
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
    uint64_t data = booleanWriter(&SIGNALS[11], SIGNALS, SIGNAL_COUNT, true,
            &send);
    data += booleanWriter(&SIGNALS[12], SIGNALS, SIGNAL_COUNT, true, &send);
    handleOccupancyMessage(SIGNALS[11].message->id, __builtin_bswap64(data),
            SIGNALS, SIGNAL_COUNT, &PIPELINE);
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
    uint64_t data = booleanWriter(&SIGNALS[11], SIGNALS, SIGNAL_COUNT, false,
            &send);
    data += booleanWriter(&SIGNALS[12], SIGNALS, SIGNAL_COUNT, false, &send);
    handleOccupancyMessage(SIGNALS[11].message->id, __builtin_bswap64(data),
            SIGNALS, SIGNAL_COUNT, &PIPELINE);
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
    CanSignal* signal = &SIGNALS[6];
    result = handleFuelFlow(signal, SIGNALS, SIGNAL_COUNT, 0, &send, 1);
    ck_assert_int_eq(0, result);
    signal->lastValue = 0;

    result = handleFuelFlow(signal, SIGNALS, SIGNAL_COUNT, 1, &send, 1);
    ck_assert_int_eq(1, result);
    signal->lastValue = 1;

    result = handleFuelFlow(signal, SIGNALS, SIGNAL_COUNT, 255, &send, 1);
    ck_assert_int_eq(255, result);
    signal->lastValue = 255;

    result = handleFuelFlow(signal, SIGNALS, SIGNAL_COUNT, 2, &send, 1);
    ck_assert_int_eq(257, result);
    signal->lastValue = 2;
}
END_TEST

START_TEST (test_door_handler)
{
    bool send = true;
    uint64_t data = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT, true,
            &send);
    sendDoorStatus("foo", __builtin_bswap64(data), &SIGNALS[2], SIGNALS,
            SIGNAL_COUNT, &PIPELINE);
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
    uint64_t data = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT, true,
            &send);
    sendDoorStatus("does-not-exist", __builtin_bswap64(data), NULL, SIGNALS,
            SIGNAL_COUNT, &PIPELINE);
    fail_unless(queueEmpty());
}
END_TEST

START_TEST (test_send_same_door_status)
{
    bool send = true;
    uint64_t data = booleanWriter(&SIGNALS[2], SIGNALS, SIGNAL_COUNT, true,
            &send);
    sendDoorStatus("driver", __builtin_bswap64(data), &SIGNALS[2], SIGNALS,
            SIGNAL_COUNT, &PIPELINE);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    sendDoorStatus("driver", __builtin_bswap64(data), &SIGNALS[2], SIGNALS,
            SIGNAL_COUNT, &PIPELINE);
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
