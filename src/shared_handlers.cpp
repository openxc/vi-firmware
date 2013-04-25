#include "shared_handlers.h"
#include "canwrite.h"
#include "log.h"

float rotationsSinceRestart = 0;
float rollingOdometerSinceRestart = 0;
float totalOdometerAtRestart = 0;
float fuelConsumedSinceRestartLiters = 0;

using openxc::can::read::decodeCanSignal;
using openxc::can::read::booleanHandler;
using openxc::can::read::stateHandler;
using openxc::can::read::sendEventedBooleanMessage;
using openxc::can::read::sendEventedFloatMessage;
using openxc::can::read::sendEventedStringMessage;
using openxc::can::read::sendNumericalMessage;
using openxc::can::read::preTranslate;
using openxc::can::read::postTranslate;
using openxc::can::write::booleanWriter;
using openxc::can::write::sendCanSignal;

const float openxc::signals::handlers::LITERS_PER_GALLON = 3.78541178;
const float openxc::signals::handlers::LITERS_PER_UL = .000001;
const float openxc::signals::handlers::KM_PER_MILE = 1.609344;
const float openxc::signals::handlers::KM_PER_M = .001;
const float openxc::signals::handlers::PI = 3.14159265;
const char* openxc::signals::handlers::DOOR_STATUS_GENERIC_NAME = "door_status";
const char* openxc::signals::handlers::BUTTON_EVENT_GENERIC_NAME = "button_event";
const char* openxc::signals::handlers::TIRE_PRESSURE_GENERIC_NAME = "tire_pressure";

void openxc::signals::handlers::sendDoorStatus(const char* doorId, uint64_t data, CanSignal* signal,
        CanSignal* signals, int signalCount, Listener* listener) {
    if(signal == NULL) {
        debug("Specific door signal for ID %s is NULL, vehicle may not support",
                doorId);
        return;
    }

    float rawAjarStatus = decodeCanSignal(signal, data);
    bool send = true;
    bool ajarStatus = booleanHandler(NULL, signals, signalCount, rawAjarStatus,
            &send);

    if(send && (signal->sendSame || !signal->received ||
                rawAjarStatus != signal->lastValue)) {
        signal->received = true;
        sendEventedBooleanMessage(DOOR_STATUS_GENERIC_NAME, doorId, ajarStatus,
                listener);
    }
    signal->lastValue = rawAjarStatus;
}

void openxc::signals::handlers::handleDoorStatusMessage(int messageId, uint64_t data, CanSignal* signals,
        int signalCount, Listener* listener) {
    sendDoorStatus("driver", data,
            lookupSignal("driver_door", signals, signalCount),
            signals, signalCount, listener);
    sendDoorStatus("passenger", data,
            lookupSignal("passenger_door", signals, signalCount),
            signals, signalCount, listener);
    sendDoorStatus("rear_right", data,
            lookupSignal("rear_right_door", signals, signalCount),
            signals, signalCount, listener);
    sendDoorStatus("rear_left", data,
            lookupSignal("rear_left_door", signals, signalCount),
            signals, signalCount, listener);
}

void openxc::signals::handlers::sendTirePressure(const char* tireId, uint64_t data, CanSignal* signal,
        CanSignal* signals, int signalCount, Listener* listener) {
    if(signal == NULL) {
        debug("Specific tire signal for ID %s is NULL, vehicle may not support",
                tireId);
        return;
    }

    bool send = true;
    // TODO use preTranslate for sendDoorStatus, too
    float pressure = preTranslate(signal, data, &send);
    if(send) {
        sendEventedFloatMessage(TIRE_PRESSURE_GENERIC_NAME, tireId, pressure,
                listener);
    }
    postTranslate(signal, pressure);
}

void openxc::signals::handlers::handleTirePressureMessage(int messageId, uint64_t data, CanSignal* signals,
        int signalCount, Listener* listener) {
    sendTirePressure("front_left", data,
            lookupSignal("tire_pressure_front_left", signals, signalCount),
            signals, signalCount, listener);
    sendTirePressure("front_right", data,
            lookupSignal("tire_pressure_front_right", signals, signalCount),
            signals, signalCount, listener);
    sendTirePressure("rear_right", data,
            lookupSignal("tire_pressure_rear_right", signals, signalCount),
            signals, signalCount, listener);
    sendTirePressure("rear_left", data,
            lookupSignal("tire_pressure_rear_left", signals, signalCount),
            signals, signalCount, listener);
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
       int signalCount, float value, bool* send, float factor) {
    if(value < signal->lastValue) {
        rollingOdometerSinceRestart += signal->maxValue - signal->lastValue
            + value;
    } else {
        rollingOdometerSinceRestart += value - signal->lastValue;
    }

    return firstReceivedOdometerValue(signals, signalCount) +
        (factor * rollingOdometerSinceRestart);
}

float openxc::signals::handlers::handleRollingOdometerKilometers(CanSignal* signal, CanSignal* signals,
       int signalCount, float value, bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send, 1);
}

float openxc::signals::handlers::handleRollingOdometerMiles(CanSignal* signal, CanSignal* signals,
       int signalCount, float value, bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send,
            KM_PER_MILE);
}

float openxc::signals::handlers::handleRollingOdometerMeters(CanSignal* signal, CanSignal* signals,
       int signalCount, float value, bool* send) {
    return handleRollingOdometer(signal, signals, signalCount, value, send,
            KM_PER_M);
}

bool openxc::signals::handlers::handleStrictBoolean(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    if(value != 0) {
        return true;
    }
    return false;
}

float openxc::signals::handlers::handleFuelFlow(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send, float multiplier) {
    if(value < signal->lastValue) {
        value = signal->maxValue - signal->lastValue + value;
    } else {
        value = value - signal->lastValue;
    }
    fuelConsumedSinceRestartLiters += multiplier * value;
    return fuelConsumedSinceRestartLiters;
}

float openxc::signals::handlers::handleFuelFlowGallons(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    return handleFuelFlow(signal, signals, signalCount, value, send,
            LITERS_PER_GALLON);
}

float openxc::signals::handlers::handleFuelFlowMicroliters(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    return handleFuelFlow(signal, signals, signalCount, value, send,
            LITERS_PER_UL);
}

float openxc::signals::handlers::handleInverted(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return value * -1;
}

void openxc::signals::handlers::handleGpsMessage(int messageId, uint64_t data, CanSignal* signals,
        int signalCount, Listener* listener) {
    float latitudeDegrees = decodeCanSignal(
            lookupSignal("latitude_degrees", signals, signalCount), data);
    float latitudeMinutes = decodeCanSignal(
            lookupSignal("latitude_minutes", signals, signalCount), data);
    float latitudeMinuteFraction = decodeCanSignal(
            lookupSignal("latitude_minute_fraction", signals, signalCount),
            data);
    float longitudeDegrees = decodeCanSignal(
            lookupSignal("longitude_degrees", signals, signalCount), data);
    float longitudeMinutes = decodeCanSignal(
            lookupSignal("longitude_minutes", signals, signalCount), data);
    float longitudeMinuteFraction = decodeCanSignal(
            lookupSignal("longitude_minute_fraction", signals, signalCount),
            data);

    latitudeMinutes = (latitudeMinutes + latitudeMinuteFraction) / 60.0;
    if(latitudeDegrees < 0) {
        latitudeMinutes *= -1;
    }
    latitudeDegrees += latitudeMinutes;

    longitudeMinutes = (longitudeMinutes + longitudeMinuteFraction) / 60.0;
    if(longitudeDegrees < 0) {
        longitudeMinutes *= -1;
    }
    longitudeDegrees += longitudeMinutes;

    sendNumericalMessage("latitude", latitudeDegrees, listener);
    sendNumericalMessage("longitude", longitudeDegrees, listener);
}

bool openxc::signals::handlers::handleExteriorLightSwitch(CanSignal* signal, CanSignal* signals,
            int signalCount, float value, bool* send) {
    return value == 2 || value == 3;
}

float openxc::signals::handlers::handleUnsignedSteeringWheelAngle(CanSignal* signal,
        CanSignal* signals, int signalCount, float value, bool* send) {
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

float openxc::signals::handlers::handleMultisizeWheelRotationCount(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send, float wheelRadius) {
    if(value < signal->lastValue) {
        rotationsSinceRestart += signal->maxValue - signal->lastValue + value;
    } else {
        rotationsSinceRestart += value - signal->lastValue;
    }
    return firstReceivedOdometerValue(signals, signalCount) + (2 * PI *
            wheelRadius * rotationsSinceRestart);
}

void openxc::signals::handlers::handleButtonEventMessage(int messageId, uint64_t data,
        CanSignal* signals, int signalCount, Listener* listener) {
    CanSignal* buttonTypeSignal = lookupSignal("button_type", signals,
            signalCount);
    CanSignal* buttonStateSignal = lookupSignal("button_state", signals,
            signalCount);

    if(buttonTypeSignal == NULL || buttonStateSignal == NULL) {
        debug("Unable to find button type and state signals");
        return;
    }

    float rawButtonType = decodeCanSignal(buttonTypeSignal, data);
    float rawButtonState = decodeCanSignal(buttonStateSignal, data);

    bool send = true;
    const char* buttonType = stateHandler(buttonTypeSignal, signals,
            signalCount, rawButtonType, &send);
    if(!send || buttonType == NULL) {
        debug("Unable to find button type corresponding to %f",
                rawButtonType);
        return;
    }

    const char* buttonState = stateHandler(buttonStateSignal, signals,
            signalCount, rawButtonState, &send);
    if(!send || buttonState == NULL) {
        debug("Unable to find button state corresponding to %f",
                rawButtonState);
        return;
    }

    sendEventedStringMessage(BUTTON_EVENT_GENERIC_NAME, buttonType,
            buttonState, listener);
}

bool openxc::signals::handlers::handleTurnSignalCommand(const char* name, cJSON* value, cJSON* event,
        CanSignal* signals, int signalCount) {
    const char* direction = value->valuestring;
    CanSignal* signal = NULL;
    if(!strcmp("left", direction)) {
        signal = lookupSignal("turn_signal_left", signals, signalCount);
    } else if(!strcmp("right", direction)) {
        signal = lookupSignal("turn_signal_right", signals, signalCount);
    }

    if(signal != NULL) {
        return sendCanSignal(signal, cJSON_CreateBool(true), booleanWriter,
                signals, signalCount, true);
    } else {
        debug("Unable to find signal for %s turn signal", direction);
    }
    return false;
}

void sendOccupancyStatus(const char* seatId, uint64_t data,
        CanSignal* lowerSignal, CanSignal* upperSignal,
        CanSignal* signals, int signalCount, Listener* listener) {
    if(lowerSignal == NULL || upperSignal == NULL) {
        debug("Upper or lower occupancy signal for seat ID %s is NULL, "
                "vehicle may not support", seatId);
        return;
    }

    float rawLowerStatus = decodeCanSignal(lowerSignal, data);
    float rawUpperStatus = decodeCanSignal(upperSignal, data);

    bool send = true;
    bool lowerStatus = booleanHandler(NULL, signals, signalCount,
            rawLowerStatus, &send);
    bool upperStatus = booleanHandler(NULL, signals, signalCount,
            rawUpperStatus, &send);
    if(lowerStatus) {
        if(upperStatus) {
            sendEventedStringMessage("occupancy_status", seatId, "adult",
                    listener);
        } else {
            sendEventedStringMessage("occupancy_status", seatId, "child",
                    listener);
        }
    } else {
        sendEventedStringMessage("occupancy_status", seatId, "empty",
                listener);
    }
}

void openxc::signals::handlers::handleOccupancyMessage(int messageId, uint64_t data, CanSignal* signals,
        int signalCount, Listener* listener) {
    sendOccupancyStatus("driver", data,
            lookupSignal("driver_occupancy_lower", signals, signalCount),
            lookupSignal("driver_occupancy_upper", signals, signalCount),
            signals, signalCount, listener);
    sendOccupancyStatus("passenger", data,
            lookupSignal("passenger_occupancy_lower", signals, signalCount),
            lookupSignal("passenger_occupancy_upper", signals, signalCount),
            signals, signalCount, listener);
}
