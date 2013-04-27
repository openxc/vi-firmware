/* Generic vehicle signal handlers useful for many different message types. */
#ifndef _SHARED_HANDLERS_H_
#define _SHARED_HANDLERS_H_

#include "can/canread.h"
#include "interface/usb.h"

using openxc::can::CanSignal;

namespace openxc {
namespace signals {
namespace handlers {

extern const float LITERS_PER_GALLON;
extern const float LITERS_PER_UL;
extern const float KM_PER_MILE;
extern const float KM_PER_M;
extern const char* DOOR_STATUS_GENERIC_NAME;
extern const char* BUTTON_EVENT_GENERIC_NAME;
extern const char* TIRE_PRESSURE_GENERIC_NAME;

#ifndef __PIC32__
extern const float PI;
#endif

/* Interpret the given signal as a wheel rotation counter, and transform it to
 * an absolute distance travelled since the car was started.
 *
 * signal - The wheel rotation count signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - The wheel rotation count parsed from the original message.
 * send - (output) Flip this to false if the message should not be sent.
 * wheelRadius - The wheel radius of the current vehicle in km.
 *
 * Returns the absolute distance travelled since the car started.
 */
float handleMultisizeWheelRotationCount(CanSignal* signal,
        CanSignal* signals, int signalCount, float value, bool* send,
        float wheelRadius);

/* Interpret the given signal as a rolling counter of km travelled, but keep
 * a log of the values and output the total km travelled since the car was
 * started. If a total odometer signal is available, take the first known value
 * of that as the baseline to give a master odometer value with higher
 * resolution
 *
 * signal - The rolling odometer signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - The rolling odometer value parsed from the original message.
 * send - (output) Flip this to false if the message should not be sent.
 *
 * Returns total km travelled since the car started.
 */
float handleRollingOdometerKilometers(CanSignal* signal, CanSignal* signals,
       int signalCount, float value, bool* send);

/* Interpret the given signal as a rolling counter of miles travelled, but keep
 * a log of the values and output the total km travelled since the car was
 * started. If a total odometer signal is available, take the first known value
 * of that as the baseline to give a master odometer value with higher
 * resolution
 *
 * signal - The rolling odometer signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - The rolling odometer value parsed from the original message.
 * send - (output) Flip this to false if the message should not be sent.
 *
 * Returns total km travelled since the car started.
 */
float handleRollingOdometerMiles(CanSignal* signal, CanSignal* signals,
       int signalCount, float value, bool* send);

/* Interpret the given signal as a rolling counter of meters travelled, but
 * keep a log of the values and output the total kilometers travelled since the
 * started. If a total odometer signal is available, take the first known value
 * of that as the baseline to give a master odometer value with higher
 * resolution
 *
 * signal - The rolling odometer signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - The rolling odometer value parsed from the original message.
 * send - (output) Flip this to false if the message should not be sent.
 *
 * Returns total km travelled since the car started.
 */
float handleRollingOdometerMeters(CanSignal* signal, CanSignal* signals,
       int signalCount, float value, bool* send);

/* Interpret the signal as a "strict" boolean - anything besides 0 is true.
 *
 * signal - The signal to interpret as a "strict" boolean.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - The numerical value parsed from the signal.
 * send - (output) Flip this to false if the message should not be sent.
 *
 * Returns false if value is 0, otherwise true.
 */
bool handleStrictBoolean(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Keep track of a rolling fuel flow counter signal (in gallons) to obtain a
 * total since the vehicle started, and convert the result from gallons to
 * liters.
 *
 * signal - The rolling fuel flow signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - Gallons of fuel used since the last rollover.
 * send - (output) Flip this to false if the message should not be sent.
 *
 * Returns the total fuel consumed since the vehicle started in liters.
 */
float handleFuelFlowGallons(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send);

/* Keep track of a rolling fuel flow counter signal (in uL) to obtain a
 * total since the vehicle started, and convert the result from uL to
 * liters.
 *
 * signal - The rolling fuel flow signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - Microliters of fuel used since the last rollover.
 * send - (output) Flip this to false if the message should not be sent.
 *
 * Returns the total fuel consumed since the vehicle started in liters.
 */
float handleFuelFlowMicroliters(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send);

/* Keep track of a rolling fuel flow counter signal to obtain a
 * total since the vehicle started, and multiply the results by the given
 * multiplier to obtain liters.
 *
 * signal - The rolling fuel flow signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - Microliters of fuel used since the last rollover.
 * send - (output) Flip this to false if the message should not be sent.
 * multiplier - factor necessary to convert the input units to liters.
 *
 * Returns the total fuel consumed since the vehicle started in liters.
 */
float handleFuelFlow(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send, float multiplier);

/* Flip the sign of the value, e.g. if the steering wheel should be negative to
 * the left and positive to the right, but the CAN signal is the opposite.
 *
 * signal - The signal to invert.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - The numerical value to invert.
 * send - (output) Flip this to false if the message should not be sent.
 *
 * Returns value with the sign flipped.
 */
float handleInverted(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Change the sign of the steering wheel angle value depending on the value of
 * another CAN signal, "steering_wheel_angle_sign".
 *
 * This is useful if steering wheel angle is received as an unsigned value, with
 * the sign stored separately.
 *
 * signal - The steering wheel angle signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * value - An unsigned steering wheel angle.
 * send - (output) Flip this to false if the message should not be sent.
 *
 * Returns a signed steering wheel angle value.
 */
float handleUnsignedSteeringWheelAngle(CanSignal* signal,
        CanSignal* signals, int signalCount, float value, bool* send);

/* Combine latitude and longitude signals split into their components (degrees,
 * minutes and fractional minutes) into 2 output message: latitude and longitude
 * with decimal precision.
 *
 * The following signals must be defined in the signal array, and they must all
 * be contained in the same CAN message:
 *
 *      * latitude_degrees
 *      * latitude_minutes
 *      * latitude_minutes_fraction
 *      * longitude_degrees
 *      * longitude_minutes
 *      * longitude_minutes_fraction
 *
 * This is a message handler, and takes care of sending the JSON messages.
 *
 * messageId - The ID of the GPS CAN message.
 * data - The CAN message data containing all GPS information.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * send - (output) Flip this to false if the message should not be sent.
 * listener - The listener that wraps the output devices.
 */
void handleGpsMessage(int messageId, uint64_t data, CanSignal* signals,
        int signalCount, Listener* listener);

/* Pull two signal out of the CAN message, "button_type" and "button_state" and
 * combine the result into a single OpenXC JSON with both a value (the button
 * ID) and an event (pressed, held, released, etc).
 *
 * messageId - The ID of the button event CAN message.
 * data - The CAN message data containing the button event information.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * send - (output) Flip this to false if the message should not be sent.
 * listener - The listener that wraps the output devices.
 */
void handleButtonEventMessage(int messageId, uint64_t data,
        CanSignal* signals, int signalCount, Listener* listener);

/* Decode a boolean signal (the door ajar status for the door in question) and
 * send an OpenXC JSON message with the value (door ID) and event (ajar status)
 * filled in.
 *
 * This function doesn't match the signature required to be used directly as a
 * message or value handler, but it can be called from another handler.
 *
 * doorId - The name of the door, e.g. rear_left.
 * data - The incoming CAN message data that contains the signal.
 * signal - The CAN signal for door status.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * listener - The listener that wraps the output devices.
 */
void sendDoorStatus(const char* doorId, uint64_t data, CanSignal* signal,
        CanSignal* signals, int signalCount, Listener* listener);

/* Decode a numerical signal (the pressure of the tire in question) and
 * send an OpenXC JSON message with the value (tire ID) and event (tire
 * pressure) filled in.
 *
 * This function doesn't match the signature required to be used directly as a
 * message or value handler, but it can be called from another handler.
 *
 * tireId - The name of the tire, e.g. rear_left.
 * data - The incoming CAN message data that contains the signal.
 * signal - The CAN signal for tire pressure.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * listener - The listener that wraps the output devices.
 */
void sendTirePressure(const char* tireId, uint64_t data, CanSignal* signal,
        CanSignal* signals, int signalCount, Listener* listener);

/**
 * We consider dipped beam or auto to be lights on.
 */
bool handleExteriorLightSwitch(CanSignal* signal, CanSignal* signals,
            int signalCount, float value, bool* send);

bool handleTurnSignalCommand(const char* name, cJSON* value, cJSON* event,
        CanSignal* signals, int signalCount);

/** Handle a CAN message that contains the ajar status of all doors.
 */
void handleDoorStatusMessage(int messageId, uint64_t data, CanSignal* signals,
        int signalCount, Listener* listener);

/** Handle a CAN message that contains the pressure of all tires.
 */
void handleTirePressureMessage(int messageId, uint64_t data, CanSignal* signals,
        int signalCount, Listener* listener);

/* Combine the values from two sensors in each seat to determine if there is
 * actually an occupant, and if so their general size (child or adult).
 *
 * The following signals must be defined in the signal array, and they must all
 * be contained in the same CAN message:
 *
 *      * passenger_occupancy_lower
 *      * passenger_occupancy_upper
 *      * TODO add more seats
 *
 * This is a message handler, and takes care of sending the JSON messages.
 *
 * messageId - The ID of the occupant sensor CAN message.
 * data - The CAN message data containing all occupancy information.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * send - (output) Flip this to false if the message should not be sent.
 * listener - The listener that wraps the output devices.
 */
void handleOccupancyMessage(int messageId, uint64_t data,
              CanSignal* signals, int signalCount, Listener* listener);

} // namespace handlers
} // namespace signals
} // namespace openxc

#endif // _SHARED_HANDLERS_H_
