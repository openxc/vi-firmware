/* Generic vehicle signal handlers useful for many different message types. */
#ifndef _SHARED_HANDLERS_H_
#define _SHARED_HANDLERS_H_

#include "can/canread.h"
#include "interface/usb.h"
#include "diagnostics.h"

namespace openxc {
namespace signals {
namespace handlers {

extern const float LITERS_PER_GALLON;
extern const float LITERS_PER_UL;
extern const float KM_PER_MILE;
extern const float KM_PER_M;
extern const char DOOR_STATUS_GENERIC_NAME[];
extern const char BUTTON_EVENT_GENERIC_NAME[];
extern const char TIRE_PRESSURE_GENERIC_NAME[];

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
 * tireRadius - The tire radius of the current vehicle in km. Tire radius is
 *      used and not wheel radius as the overall tire radius tends to be more
 *      consistent across vehicles in the same model, while the wheel size may
 *      change.
 *
 * Returns the absolute distance travelled since the car started.
 */
openxc_DynamicField handleMultisizeWheelRotationCount(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, float value, bool* send,
        float tireRadius);

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
openxc_DynamicField handleRollingOdometerKilometers(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
openxc_DynamicField handleRollingOdometerMiles(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
openxc_DynamicField handleRollingOdometerMeters(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
openxc_DynamicField handleStrictBoolean(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
openxc_DynamicField handleFuelFlowGallons(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
openxc_DynamicField handleFuelFlowMicroliters(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
openxc_DynamicField handleFuelFlow(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, float value, bool* send, float multiplier);

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
openxc_DynamicField handleInverted(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
openxc_DynamicField handleUnsignedSteeringWheelAngle(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

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
 * pipeline - The pipeline that wraps the output devices.
 */
void handleGpsMessage(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, CanMessage* message, openxc::pipeline::Pipeline* pipeline);

/* Pull two signal out of the CAN message, "button_type" and "button_state" and
 * combine the result into a single OpenXC JSON with both a value (the button
 * ID) and an event (pressed, held, released, etc).
 *
 * messageId - The ID of the button event CAN message.
 * data - The CAN message data containing the button event information.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * send - (output) Flip this to false if the message should not be sent.
 * pipeline - The pipeline that wraps the output devices.
 */
void handleButtonEventMessage(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, CanMessage* message, openxc::pipeline::Pipeline* pipeline);

/**
 * We consider dipped beam or auto to be lights on.
 */
openxc_DynamicField handleExteriorLightSwitch(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount, openxc::pipeline::Pipeline* pipeline, float value, bool* send);

void handleTurnSignalCommand(const char* name, openxc_DynamicField* value,
              openxc_DynamicField* event, const CanSignal* signals, int signalCount);

/* Public: Decode a door status from a signal and send it out as an evented
 * simple vehicle message, e.g. :
 *
 *      name: door_status
 *      value: <the door>
 *      event: bool
 *
 * This returns the door ID it found for the signal and marks the 'send' output
 * parameter as false, indicating to the pipeline that this signal has already
 * been handled.
 */
openxc_DynamicField doorStatusDecoder(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount,
       openxc::pipeline::Pipeline* pipeline, float value, bool* send);

/* Public: Decode a tire pressure from a signal and send it out as an evented
 * simple vehicle message, e.g. :
 *
 *      name: tire_pressure
 *      value: <the tire>
 *      event: <the pressure, just the raw signal value with normal factor and
     *      offset>
 *
 * This returns the tire it found for the signal and marks the 'send' output
 * parameter as false, indicating to the pipeline that this signal has already
 * been handled.
 */
openxc_DynamicField tirePressureDecoder(const CanSignal* signal, const CanSignal* signals, SignalManager* signalManager,
        SignalManager* signalManagers, int signalCount,
       openxc::pipeline::Pipeline* pipeline, float value, bool* send);

} // namespace handlers
} // namespace signals
} // namespace openxc

#endif // _SHARED_HANDLERS_H_
