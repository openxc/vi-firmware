#include "shared_handlers.h"
#include "can/canwrite.h"
#include "payload/payload.h"
#include "util/log.h"

#define OCCUPANCY_STATUS_GENERIC_NAME "occupancy_status"
#define PSI_PER_KPA 0.145037738

float rotationsSinceRestart = 0;
float rollingOdometerSinceRestart = 0;
float totalOdometerAtRestart = 0;
float fuelConsumedSinceRestartLiters = 0;

namespace can = openxc::can;

using openxc::util::log::debug;
using openxc::can::read::stateDecoder;
using openxc::can::read::publishVehicleMessage;
using openxc::can::read::publishNumericalMessage;
using openxc::can::read::translateSignal;
using openxc::can::read::parseSignalBitfield;
using openxc::can::read::shouldSend;
using openxc::can::lookupSignal;
using openxc::pipeline::Pipeline;

const float openxc::signals::handlers::LITERS_PER_GALLON = 3.78541178;
const float openxc::signals::handlers::LITERS_PER_UL = .000001;
const float openxc::signals::handlers::KM_PER_MILE = 1.609344;
const float openxc::signals::handlers::KM_PER_M = .001;
const char openxc::signals::handlers::DOOR_STATUS_GENERIC_NAME[] = "door_status";
const char openxc::signals::handlers::BUTTON_EVENT_GENERIC_NAME[] =
        "button_event";
const char openxc::signals::handlers::TIRE_PRESSURE_GENERIC_NAME[] =
        "tire_pressure";

#ifndef __PIC32__
const float openxc::signals::handlers::PI = 3.14159265;
#endif

openxc_DynamicField openxc::signals::handlers::doorStatusDecoder(CanSignal* signal,
        CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField ajarStatus = can::read::booleanDecoder(signal, signals,
            signalCount, pipeline, value, send);
    openxc_DynamicField doorIdValue = openxc_DynamicField();		// 0 fill the structure
    // Must manually check if the signal should send (e.g. based on send_same
    // attribute of the signal) since this decoder handles sending the message
    // itself instead of letting the caller do that.
    if(send && shouldSend(signal, value)) {
        doorIdValue.type = openxc_DynamicField_Type_STRING;
        if(!strcmp(signal->genericName, "driver_door")) {
            strcpy(doorIdValue.string_value, "driver");
        } else if(!strcmp(signal->genericName, "passenger_door")) {
            strcpy(doorIdValue.string_value, "passenger");
        } else if(!strcmp(signal->genericName, "rear_right_door")) {
            strcpy(doorIdValue.string_value, "rear_right");
        } else if(!strcmp(signal->genericName, "rear_left_door")) {
            strcpy(doorIdValue.string_value, "rear_left");
        } else {
            strcpy(doorIdValue.string_value, signal->genericName);
        }
        publishVehicleMessage(DOOR_STATUS_GENERIC_NAME, &doorIdValue,
                &ajarStatus, pipeline);
    }

    // Block the normal sending, we overrode it
    *send = false;
    return doorIdValue;
}

openxc_DynamicField openxc::signals::handlers::tirePressureDecoder(
        CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField pressure = payload::wrapNumber(value);
    openxc_DynamicField tireIdValue = openxc_DynamicField();	// zero fill
    // Must manually check if the signal should send (e.g. based on send_same
    // attribute of the signal) since this decoder handles sending the message
    // itself instead of letting the caller do that.
    if(send && shouldSend(signal, value)) {
        tireIdValue.type = openxc_DynamicField_Type_STRING;
        if(!strcmp(signal->genericName, "tire_pressure_front_left")) {
            strcpy(tireIdValue.string_value, "front_left");
        } else if(!strcmp(signal->genericName, "tire_pressure_front_right")) {
            strcpy(tireIdValue.string_value, "front_right");
        } else if(!strcmp(signal->genericName, "tire_pressure_rear_left")) {
            strcpy(tireIdValue.string_value, "rear_right");
        } else if(!strcmp(signal->genericName, "tire_pressure_rear_right")) {
            strcpy(tireIdValue.string_value, "rear_left");
        } else {
            strcpy(tireIdValue.string_value, signal->genericName);
        }

        publishVehicleMessage(TIRE_PRESSURE_GENERIC_NAME, &tireIdValue,
                &pressure, pipeline);
    }

    // Block the normal sending, we overrode it
    *send = false;
    return tireIdValue;
}

float firstReceivedOdometerValue(CanSignal* signals, int signalCount) {
    if(totalOdometerAtRestart == 0) {
        CanSignal* odometerSignal = lookupSignal("total_odometer", signals,
                signalCount);
        if(odometerSignal != NULL && odometerSignal->received) {
            totalOdometerAtRestart = odometerSignal->lastValue;
        }
    }
    return totalOdometerAtRestart;
}

openxc_DynamicField handleRollingOdometer(CanSignal* signal, CanSignal* signals,
       int signalCount, float value, bool* send,
       float factor) {
    if(value < signal->lastValue) {
        rollingOdometerSinceRestart += signal->maxValue - signal->lastValue
            + value;
    } else {
        rollingOdometerSinceRestart += value - signal->lastValue;
    }

    return openxc::payload::wrapNumber(firstReceivedOdometerValue(signals, signalCount) +
        (factor * rollingOdometerSinceRestart));
}

openxc_DynamicField openxc::signals::handlers::handleRollingOdometerKilometers(
        CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send, 1);
}

openxc_DynamicField openxc::signals::handlers::handleRollingOdometerMiles(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send,
            KM_PER_MILE);
}

openxc_DynamicField openxc::signals::handlers::handleRollingOdometerMeters(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send,
            KM_PER_M);
}

openxc_DynamicField openxc::signals::handlers::handleStrictBoolean(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return openxc::payload::wrapBoolean(value != 0);
}

openxc_DynamicField openxc::signals::handlers::handleFuelFlow(CanSignal* signal,
        CanSignal* signals, int signalCount, float value,
        bool* send, float multiplier) {
    if(value < signal->lastValue) {
        value = signal->maxValue - signal->lastValue + value;
    } else {
        value = value - signal->lastValue;
    }
    fuelConsumedSinceRestartLiters += multiplier * value;
    return openxc::payload::wrapNumber(fuelConsumedSinceRestartLiters);
}

openxc_DynamicField openxc::signals::handlers::handleFuelFlowGallons(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return handleFuelFlow(signal, signals, signalCount, value, send,
            LITERS_PER_GALLON);
}

openxc_DynamicField openxc::signals::handlers::handleFuelFlowMicroliters(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return handleFuelFlow(signal, signals, signalCount, value, send,
            LITERS_PER_UL);
}

openxc_DynamicField openxc::signals::handlers::handleInverted(CanSignal* signal, CanSignal*
        signals, int signalCount, Pipeline* pipeline, float value, bool* send) {
    return openxc::payload::wrapNumber(value * -1);
}

void openxc::signals::handlers::handleGpsMessage(CanMessage* message,
        CanSignal* signals, int signalCount, Pipeline* pipeline) {
    CanSignal* latitudeDegreesSignal =
        lookupSignal("latitude_degrees", signals, signalCount);
    CanSignal* latitudeMinutesSignal =
        lookupSignal("latitude_minutes", signals, signalCount);
    CanSignal* latitudeMinuteFractionSignal =
        lookupSignal("latitude_minute_fraction", signals, signalCount);
    CanSignal* longitudeDegreesSignal =
        lookupSignal("longitude_degrees", signals, signalCount);
    CanSignal* longitudeMinutesSignal =
        lookupSignal("longitude_minutes", signals, signalCount);
    CanSignal* longitudeMinuteFractionSignal =
        lookupSignal("longitude_minute_fraction", signals, signalCount);

    if(latitudeDegreesSignal == NULL ||
            latitudeMinutesSignal == NULL ||
            latitudeMinuteFractionSignal == NULL ||
            longitudeDegreesSignal == NULL ||
            longitudeMinutesSignal == NULL ||
            longitudeMinuteFractionSignal == NULL) {
        debug("One or more GPS signals are missing, no GPS");
        return;
    }

    float latitudeDegrees = parseSignalBitfield(latitudeDegreesSignal, message);
    float latitudeMinutes = parseSignalBitfield(latitudeMinutesSignal, message);
    float latitudeMinuteFraction = parseSignalBitfield(
            latitudeMinuteFractionSignal, message);
    float longitudeDegrees = parseSignalBitfield(longitudeDegreesSignal, message);
    float longitudeMinutes = parseSignalBitfield(longitudeMinutesSignal, message);
    float longitudeMinuteFraction = parseSignalBitfield(
            longitudeMinuteFractionSignal, message);

    float latitude = (latitudeMinutes + latitudeMinuteFraction) / 60.0;
    if(latitudeDegrees < 0) {
        latitude *= -1;
    }
    latitude += latitudeDegrees;

    float longitude = (longitudeMinutes + longitudeMinuteFraction) / 60.0;
    if(longitudeDegrees < 0) {
        longitude *= -1;
    }
    longitude += longitudeDegrees;

    publishNumericalMessage("latitude", latitude, pipeline);
    publishNumericalMessage("longitude", longitude, pipeline);
}

openxc_DynamicField openxc::signals::handlers::handleExteriorLightSwitch(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return openxc::payload::wrapBoolean(value == 2 || value == 3);
}

openxc_DynamicField openxc::signals::handlers::handleUnsignedSteeringWheelAngle(
        CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    CanSignal* steeringAngleSign = lookupSignal("steering_wheel_angle_sign",
            signals, signalCount);

    if(steeringAngleSign == NULL) {
        debug("Unable to find stering wheel angle sign signal");
        *send = false;
    } else {
        if(steeringAngleSign->lastValue == 0) {
            // left turn
            value *= -1;
        }
    }
    return openxc::payload::wrapNumber(value);
}

openxc_DynamicField openxc::signals::handlers::handleMultisizeWheelRotationCount(
        CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send, float tireRadius) {
    if(value < signal->lastValue) {
        rotationsSinceRestart += signal->maxValue - signal->lastValue + value;
    } else {
        rotationsSinceRestart += value - signal->lastValue;
    }
    return openxc::payload::wrapNumber(firstReceivedOdometerValue(signals,
            signalCount) + (2 * PI * tireRadius * rotationsSinceRestart));
}

void openxc::signals::handlers::handleButtonEventMessage(CanMessage* message,
        CanSignal* signals, int signalCount, Pipeline* pipeline) {
    CanSignal* buttonTypeSignal = lookupSignal("button_type", signals,
            signalCount);
    CanSignal* buttonStateSignal = lookupSignal("button_state", signals,
            signalCount);

    if(buttonTypeSignal == NULL || buttonStateSignal == NULL) {
        debug("Unable to find button type and state signals");
        return;
    }

    bool send = true;
    float rawButtonType = parseSignalBitfield(buttonTypeSignal, message);
    float rawButtonState = parseSignalBitfield(buttonStateSignal, message);

    openxc_DynamicField buttonType = stateDecoder(buttonTypeSignal,
            signals, signalCount, pipeline, rawButtonType, &send);
    if(!send) {
        debug("Unable to find button type corresponding to %f",
                rawButtonType);
    } else {
        openxc_DynamicField buttonState = stateDecoder(buttonStateSignal,
                signals, signalCount, pipeline, rawButtonState, &send);
        if(!send) {
            debug("Unable to find button state corresponding to %f",
                    rawButtonState);
        } else {
            publishVehicleMessage(BUTTON_EVENT_GENERIC_NAME, &buttonType,
                    &buttonState, pipeline);
        }
    }
}

void openxc::signals::handlers::handleTurnSignalCommand(const char* name,
        openxc_DynamicField* value, openxc_DynamicField* event,
        CanSignal* signals, int signalCount) {
    const char* direction = value->string_value;
    CanSignal* signal = NULL;
    if(!strcmp("left", direction)) {
        signal = lookupSignal("turn_signal_left", signals, signalCount);
    } else if(!strcmp("right", direction)) {
        signal = lookupSignal("turn_signal_right", signals, signalCount);
    }

    if(signal != NULL) {
        can::write::encodeAndSendBooleanSignal(signal, true, true);
    } else {
        debug("Unable to find signal for %s turn signal", direction);
    }
}
