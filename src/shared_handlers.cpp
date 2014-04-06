#include "shared_handlers.h"
#include "can/canwrite.h"
#include "util/log.h"

#define OCCUPANCY_STATUS_GENERIC_NAME "occupancy_status"
#define PSI_PER_KPA 0.145037738

float rotationsSinceRestart = 0;
float rollingOdometerSinceRestart = 0;
float totalOdometerAtRestart = 0;
float fuelConsumedSinceRestartLiters = 0;

namespace can = openxc::can;

using openxc::util::log::debug;
using openxc::can::read::booleanDecoder;
using openxc::can::read::stateDecoder;
using openxc::can::read::noopDecoder;
using openxc::can::read::publishVehicleMessage;
using openxc::can::read::publishNumericalMessage;
using openxc::can::read::preTranslate;
using openxc::can::read::postTranslate;
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

void openxc::signals::handlers::sendDoorStatus(const char* doorId,
        uint8_t data[], CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline) {
    if(signal == NULL) {
        // debug("Specific door signal for ID %s is NULL, vehicle may not support",
                // doorId);
        return;
    }

    bool send = true;
    float rawAjarStatus = preTranslate(signal, data, &send);
    openxc_DynamicField ajarStatus = booleanDecoder(signal, signals,
            signalCount, pipeline, rawAjarStatus, &send);
    if(send) {
        openxc_DynamicField doorIdValue = {0};
        doorIdValue.has_type = true;
        doorIdValue.type = openxc_DynamicField_Type_STRING;
        doorIdValue.has_string_value = true;
        strcpy(doorIdValue.string_value, doorId);
        publishVehicleMessage(DOOR_STATUS_GENERIC_NAME, &doorIdValue,
                &ajarStatus, pipeline);
    }
    postTranslate(signal, rawAjarStatus);
}

void openxc::signals::handlers::handleDoorStatusMessage(int messageId,
        uint8_t data[], CanSignal* signals, int signalCount,
        Pipeline* pipeline) {
    sendDoorStatus("driver", data,
            lookupSignal("driver_door", signals, signalCount),
            signals, signalCount, pipeline);
    sendDoorStatus("passenger", data,
            lookupSignal("passenger_door", signals, signalCount),
            signals, signalCount, pipeline);
    sendDoorStatus("rear_right", data,
            lookupSignal("rear_right_door", signals, signalCount),
            signals, signalCount, pipeline);
    sendDoorStatus("rear_left", data,
            lookupSignal("rear_left_door", signals, signalCount),
            signals, signalCount, pipeline);
    sendDoorStatus("hood", data,
            lookupSignal("hood", signals, signalCount),
            signals, signalCount, pipeline);
    sendDoorStatus("trunk", data,
            lookupSignal("trunk", signals, signalCount),
            signals, signalCount, pipeline);
}

void openxc::signals::handlers::sendTirePressure(const char* tireId,
        uint8_t data[], float conversionFactor, CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline) {
    if(signal == NULL) {
        debug("Specific tire signal for ID %s is NULL, vehicle may not support",
                tireId);
        return;
    }

    bool send = true;
    float rawPressure = preTranslate(signal, data, &send);
    openxc_DynamicField pressure = noopDecoder(signal, signals, signalCount,
            pipeline, rawPressure * conversionFactor, &send);
    if(send) {
        openxc_DynamicField tireIdValue = {0};
        tireIdValue.has_type = true;
        tireIdValue.type = openxc_DynamicField_Type_STRING;
        tireIdValue.has_string_value = true;
        strcpy(tireIdValue.string_value, tireId);

        publishVehicleMessage(TIRE_PRESSURE_GENERIC_NAME, &tireIdValue,
                &pressure, pipeline);
    }
    postTranslate(signal, rawPressure);
}

void handleTirePressureMessage(int messageId,
        uint8_t data[], int conversionFactor, CanSignal* signals,
        int signalCount, Pipeline* pipeline) {
    openxc::signals::handlers::sendTirePressure("front_left", data,
            conversionFactor,
            lookupSignal("tire_pressure_front_left", signals, signalCount),
            signals, signalCount, pipeline);
    openxc::signals::handlers::sendTirePressure("front_right", data,
            conversionFactor,
            lookupSignal("tire_pressure_front_right", signals, signalCount),
            signals, signalCount, pipeline);
    openxc::signals::handlers::sendTirePressure("rear_right", data,
            conversionFactor,
            lookupSignal("tire_pressure_rear_right", signals, signalCount),
            signals, signalCount, pipeline);
    openxc::signals::handlers::sendTirePressure("rear_left", data,
            conversionFactor,
            lookupSignal("tire_pressure_rear_left", signals, signalCount),
            signals, signalCount, pipeline);
}

void openxc::signals::handlers::handlePsiTirePressureMessage(int messageId,
        uint8_t data[], CanSignal* signals, int signalCount,
        Pipeline* pipeline) {
    handleTirePressureMessage(messageId, data, 1, signals, signalCount,
            pipeline);
}

void openxc::signals::handlers::handleKpaTirePressureMessage(int messageId,
        uint8_t data[], CanSignal* signals, int signalCount,
        Pipeline* pipeline) {
    handleTirePressureMessage(messageId, data, PSI_PER_KPA, signals,
            signalCount, pipeline);
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

float handleRollingOdometer(CanSignal* signal, CanSignal* signals,
       int signalCount, float value, bool* send,
       float factor) {
    if(value < signal->lastValue) {
        rollingOdometerSinceRestart += signal->maxValue - signal->lastValue
            + value;
    } else {
        rollingOdometerSinceRestart += value - signal->lastValue;
    }

    return firstReceivedOdometerValue(signals, signalCount) +
        (factor * rollingOdometerSinceRestart);
}

float openxc::signals::handlers::handleRollingOdometerKilometers(
        CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send, 1);
}

float openxc::signals::handlers::handleRollingOdometerMiles(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send,
            KM_PER_MILE);
}

float openxc::signals::handlers::handleRollingOdometerMeters(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send,
            KM_PER_M);
}

bool openxc::signals::handlers::handleStrictBoolean(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return value != 0;
}

float openxc::signals::handlers::handleFuelFlow(CanSignal* signal,
        CanSignal* signals, int signalCount, float value,
        bool* send, float multiplier) {
    if(value < signal->lastValue) {
        value = signal->maxValue - signal->lastValue + value;
    } else {
        value = value - signal->lastValue;
    }
    fuelConsumedSinceRestartLiters += multiplier * value;
    return fuelConsumedSinceRestartLiters;
}

float openxc::signals::handlers::handleFuelFlowGallons(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return handleFuelFlow(signal, signals, signalCount, value, send,
            LITERS_PER_GALLON);
}

float openxc::signals::handlers::handleFuelFlowMicroliters(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return handleFuelFlow(signal, signals, signalCount, value, send,
            LITERS_PER_UL);
}

float openxc::signals::handlers::handleInverted(CanSignal* signal, CanSignal*
        signals, int signalCount, Pipeline* pipeline, float value, bool* send) {
    return value * -1;
}

void openxc::signals::handlers::handleGpsMessage(int messageId, uint8_t data[],
        CanSignal* signals, int signalCount, Pipeline* pipeline) {
    bool send = true;
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

    float latitudeDegrees = preTranslate(latitudeDegreesSignal, data, &send);
    float latitudeMinutes = preTranslate(latitudeMinutesSignal, data, &send);
    float latitudeMinuteFraction = preTranslate(
            latitudeMinuteFractionSignal, data, &send);
    float longitudeDegrees = preTranslate(longitudeDegreesSignal, data, &send);
    float longitudeMinutes = preTranslate(longitudeMinutesSignal, data, &send);
    float longitudeMinuteFraction = preTranslate(
            longitudeMinuteFractionSignal, data, &send);

    if(send) {
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

    postTranslate(latitudeDegreesSignal, latitudeDegrees);
    postTranslate(latitudeMinutesSignal, latitudeMinutes);
    postTranslate(latitudeMinuteFractionSignal, latitudeMinuteFraction);
    postTranslate(longitudeDegreesSignal, longitudeDegrees);
    postTranslate(longitudeMinutesSignal, longitudeMinutes);
    postTranslate(longitudeMinuteFractionSignal, longitudeMinuteFraction);
}

bool openxc::signals::handlers::handleExteriorLightSwitch(CanSignal* signal,
        CanSignal* signals, int signalCount, Pipeline* pipeline, float value,
        bool* send) {
    return value == 2 || value == 3;
}

float openxc::signals::handlers::handleUnsignedSteeringWheelAngle(CanSignal*
        signal, CanSignal* signals, int signalCount, Pipeline* pipeline,
        float value, bool* send) {
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
    return value;
}

float openxc::signals::handlers::handleMultisizeWheelRotationCount(
        CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send, float wheelRadius) {
    if(value < signal->lastValue) {
        rotationsSinceRestart += signal->maxValue - signal->lastValue + value;
    } else {
        rotationsSinceRestart += value - signal->lastValue;
    }
    return firstReceivedOdometerValue(signals, signalCount) + (2 * PI *
            wheelRadius * rotationsSinceRestart);
}

void openxc::signals::handlers::handleButtonEventMessage(int messageId,
        uint8_t data[], CanSignal* signals, int signalCount,
        Pipeline* pipeline) {
    CanSignal* buttonTypeSignal = lookupSignal("button_type", signals,
            signalCount);
    CanSignal* buttonStateSignal = lookupSignal("button_state", signals,
            signalCount);

    if(buttonTypeSignal == NULL || buttonStateSignal == NULL) {
        debug("Unable to find button type and state signals");
        return;
    }

    bool send = true;
    float rawButtonType = preTranslate(buttonTypeSignal, data, &send);
    float rawButtonState = preTranslate(buttonStateSignal, data, &send);

    openxc_DynamicField buttonType = stateDecoder(buttonTypeSignal, signals,
            signalCount, pipeline, rawButtonType, &send);
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
    postTranslate(buttonTypeSignal, rawButtonType);
    postTranslate(buttonStateSignal, rawButtonState);
}

bool openxc::signals::handlers::handleTurnSignalCommand(const char* name,
        openxc_DynamicField* value, openxc_DynamicField* event,
        CanSignal* signals, int signalCount) {
    const char* direction = value->string_value;
    CanSignal* signal = NULL;
    if(!strcmp("left", direction)) {
        signal = lookupSignal("turn_signal_left", signals, signalCount);
    } else if(!strcmp("right", direction)) {
        signal = lookupSignal("turn_signal_right", signals, signalCount);
    }

    bool sent = true;
    if(signal != NULL) {
        can::write::encodeAndSendBooleanSignal(signal, true, true);
    } else {
        debug("Unable to find signal for %s turn signal", direction);
        sent = false;
    }
    return sent;
}

void sendOccupancyStatus(const char* seatId, uint8_t data[],
        CanSignal* lowerSignal, CanSignal* upperSignal,
        CanSignal* signals, int signalCount, Pipeline* pipeline) {
    if(lowerSignal == NULL || upperSignal == NULL) {
        debug("Upper or lower occupancy signal for seat ID %s is NULL, "
                "vehicle may not support", seatId);
        return;
    }

    bool send = true;
    float rawLowerStatus = preTranslate(lowerSignal, data, &send);
    float rawUpperStatus = preTranslate(upperSignal, data, &send);

    openxc_DynamicField lowerStatus = booleanDecoder(NULL, signals, signalCount,
            pipeline, rawLowerStatus, &send);
    openxc_DynamicField upperStatus = booleanDecoder(NULL, signals, signalCount,
            pipeline, rawUpperStatus, &send);
    if(send) {
        openxc_DynamicField seatIdValue = {0};
        seatIdValue.has_type = true;
        seatIdValue.type = openxc_DynamicField_Type_STRING;
        seatIdValue.has_string_value = true;
        strcpy(seatIdValue.string_value, seatId);

        openxc_DynamicField occupancyEvent = {0};
        occupancyEvent.has_type = true;
        occupancyEvent.type = openxc_DynamicField_Type_STRING;
        occupancyEvent.has_string_value = true;

        if(lowerStatus.boolean_value) {
            if(upperStatus.boolean_value) {
                strcpy(occupancyEvent.string_value, "adult");
            } else {
                strcpy(occupancyEvent.string_value, "child");
            }
        } else {
            strcpy(occupancyEvent.string_value, "empty");
        }
        publishVehicleMessage(OCCUPANCY_STATUS_GENERIC_NAME,
                &seatIdValue, &occupancyEvent, pipeline);
    }
    postTranslate(lowerSignal, rawLowerStatus);
    postTranslate(upperSignal, rawUpperStatus);
}

void openxc::signals::handlers::handleOccupancyMessage(int messageId,
        uint8_t data[], CanSignal* signals, int signalCount,
        Pipeline* pipeline) {
    sendOccupancyStatus("driver", data,
            lookupSignal("driver_occupancy_lower", signals, signalCount),
            lookupSignal("driver_occupancy_upper", signals, signalCount),
            signals, signalCount, pipeline);
    sendOccupancyStatus("passenger", data,
            lookupSignal("passenger_occupancy_lower", signals, signalCount),
            lookupSignal("passenger_occupancy_upper", signals, signalCount),
            signals, signalCount, pipeline);
}
