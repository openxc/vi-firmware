#ifndef _CANUTIL_H_
#define _CANUTIL_H_

#include "WProgram.h"
#include "chipKITCAN.h"
#include "bitfield.h"
#include "usbutil.h"

#define SYS_FREQ (80000000L)

/* Network Node Addresses */
#define CAN_1_ADDRESS 0x101
#define CAN_2_ADDRESS 0x102

/* Public: A CAN transceiver message filter.
 *
 * number - the ID of this filter, e.g. 0, 1, 2.
 * value - the filter's value.
 * channel - the CAN channel this filter should be applied to.
 * maskNumber - the ID of the mask this filter should be paired with.
 */
struct CanFilter {
    int number;
    int value;
    int channel;
    int maskNumber;
};

/* Public: A state-based (SED) signal's mapping from numerical values to OpenXC
 * state names.
 *
 * value - the integer value of the state on the CAN bus.
 * name  - the corresponding string name for the state in OpenXC>
 */
struct CanSignalState {
    int value;
    char* name;
};

/* Public: A CAN signal to decode from the bus and output over USB.
 *
 * id          - the ID of the signal on the bus.
 * genericName - the name of the signal to be output over USB.
 * bitPosition - the starting bit of the signal in its CAN message.
 * bitSize     - the width of the bit field in the CAN message.
 * factor      - the final value will be multiplied by this factor.
 * offset      - the final value will be added to this offset.
 */
struct CanSignal {
    int id;
    char* genericName;
    int bitPosition;
    int bitSize;
    float factor;
    float offset;
    float minValue;
    float maxValue;
    CanSignalState* states;
    int stateCount;
    float lastValue;
};

/* Public: Initializes message filters on the CAN controller.
 *
 * canMod - a pointer to an initialized CAN module class.
 * filters - an array of filters to initialize.
 */
void configureFilters(CAN *canMod, CanFilter* filters, int filterCount);

/* Public: Parses a CAN signal from a CAN message, applies required
 *         transforations and sends the result over USB.
 *
 * usbDevice - the USB device to send the final formatted message on.
 * signal - the details of the signal to decode and forward.
 * data   - the raw bytes of the CAN message that contains the signal.
 */
void translateCanSignal(USBDevice* usbDevice, CanSignal* signal, uint8_t* data,
        CanSignal* signals, int signalCount);

/* Public: Parses a CAN signal from a CAN message, applies required
 *         transforations and also runs the final float value through the
 *         handler function before sending the result out over USB.
 *
 * usbDevice - the USB device to send the final formatted message on.
 * signal        - the details of the signal to decode and forward.
 * data          - the raw bytes of the CAN message that contains the signal.
 * handler - a function pointer that performs extra processing on the
 *                 float value.
 * signals       - an array of all active signals.
 * signalCount   - the length of the signals array
 */
void translateCanSignal(USBDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

void translateCanSignal(USBDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

void translateCanSignal(USBDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

/* Public: Parses a CAN signal from a message and applies required
 *           transformation.
 *
 * signal - the details of the signal to decode and forward.
 * data   - the raw bytes of the CAN message that contains the signal.
 *
 * Returns the final, transformed value of the signal.
 */
float decodeCanSignal(CanSignal* signal, uint8_t* data);

void sendNumericalMessage(char* name, float value, USBDevice* usbDevice);

/* Public: Finds and returns the corresponding string state for an integer
 *         value.
 *
 * signal  - the details of the signal that contains the state mapping.
 * signals - the list of all signals
 * signalCount - the length of the signals array
 * value   - the numerical value that maps to a state
 */
char* stateHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Public: Coerces a numerical value to a boolean.
 *
 * signal  - the details of the signal that contains the state mapping.
 * signals - the list of all signals
 * signalCount - the length of the signals array
 * value   - the numerical value that will be converted to a boolean.
 */
bool booleanHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Public: Store the value of a signal, but flip the send flag to false.
 *
 * signal  - the details of the signal that contains the state mapping.
 * signals - the list of all signals
 * signalCount - the length of the signals array
 * value   - the numerical value that will be converted to a boolean.
 */
float ignoreHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send);

/* Public: Look up the CanSignal representation of a signal based on its generic
 *         name.
 *
 * name - the generic, OpenXC name of the signal
 * signals - the list of all signals
 * signalCount - the length of the signals array
 *
 * Returns a pointer to the CanSignal if found, otherwise null;
 */
CanSignal* lookupSignal(char* name, CanSignal* signals, int signalCount);

/* Initialize the CAN controller. See inline comments for description of the
 * process.
 */
void initializeCan(CAN* bus, int address, int speed, uint8_t* messageArea);

#endif // _CANUTIL_H_
