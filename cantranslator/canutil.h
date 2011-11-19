#ifndef _CANUTIL_H_
#define _CANUTIL_H_

#include "WProgram.h"
#include "chipKITCAN.h"
#include "bitfield.h"

/* Public: A CAN transceiver message filter mask.
 *
 * number - the ID of this mask, e.g. 0, 1, 2, 3. This is neccessary to link
 *     filters with the masks they match.
 * value - the value of the mask, e.g. 0x7ff.
 */
struct CanFilterMask {
    int number;
    int value;
};

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
    CanSignalState* states;
    int stateCount;
    int lastValue;
};

/* Public: Initializes message filter masks and filters on the CAN controller.
 *
 * canMod - a pointer to an initialized CAN module class.
 * filterMasks - an array of the filter masks to initialize.
 * filters - an array of filters to initialize.
 */
void configureFilters(CAN *canMod, CanFilterMask* filterMasks,
    CanFilter* filters);

/* Public: Parses a CAN signal from a CAN message, applies required
 *         transforations and sends the result over USB.
 *
 * signal - the details of the signal to decode and forward.
 * data   - the raw bytes of the CAN message that contains the signal.
 */
void translateCanSignal(CanSignal* signal, uint8_t* data, CanSignal* signals);

/* Public: Parses a CAN signal from a CAN message, applies required
 *         transforations and also runs the final float value through the
 *         customHandler function before sending the result out over USB.
 *
 * signal        - the details of the signal to decode and forward.
 * data          - the raw bytes of the CAN message that contains the signal.
 * customHandler - a function pointer that performs extra processing on the
 *                 float value.
 * signals       - an array of all active signals.
 */
void translateCanSignal(CanSignal* signal, uint8_t* data,
        char* (*customHandler)(CanSignal*, CanSignal*, float),
        CanSignal* signals);

void translateCanSignal(CanSignal* signal, uint8_t* data,
        float (*customHandler)(CanSignal*, CanSignal*, float),
        CanSignal* signals);

void translateCanSignal(CanSignal* signal, uint8_t* data,
        bool (*customHandler)(CanSignal*, CanSignal*, float),
        CanSignal* signals);

/* Public: Parses a CAN signal from a message and applies required
 *           transformation.
 *
 * signal - the details of the signal to decode and forward.
 * data   - the raw bytes of the CAN message that contains the signal.
 *
 * Returns the final, transformed value of the signal.
 */
float decodeCanSignal(CanSignal* signal, uint8_t* data);

/* Public: Constructs a JSON representation of the translated signal.
 *
 * signal - the CAN signal this value is an instance of.
 * value  - the final, translated value for the signal.
 *
 * Returns JSON in a string.
 */
char* generateJson(CanSignal* signal, float value);
char* generateJson(CanSignal* signal, char* value);
char* generateJson(CanSignal* signal, bool value);

#endif // _CANUTIL_H_
