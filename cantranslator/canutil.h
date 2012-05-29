#ifndef _CANUTIL_H_
#define _CANUTIL_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bitfield.h"
#include "chipKITCAN.h"
#include "cJSON.h"

#define BUS_MEMORY_BUFFER_SIZE 2 * 8 * 16

/* Public: A container for a CAN module paried with a certain bus.
 *
 * speed - The bus speed in bits per second (e.g. 500000)
 * address - The address or ID of this node
 * bus - a reference to the CAN module from the CAN library
 * interruptHandler - a function to call by the Interrupt Service Routine when
 *      a previously registered CAN event occurs.
 * buffer - message area for 2 channels to store 8 16 byte messages.
 * messageReceived - used as an event flags by the interrupt service routines.
 */
struct CanBus {
    unsigned int speed;
    uint64_t address;
    CAN* bus;
    void (*interruptHandler)();
    uint8_t buffer[BUS_MEMORY_BUFFER_SIZE];
    volatile bool messageReceived;
};

/* Public: A CAN transceiver message filter mask.
 *
 * number - The ID of this mask, e.g. 0, 1, 2, 3. This is neccessary to link
 *     filters with the masks they match.
 * value - The value of the mask, e.g. 0x7ff.
 */
struct CanFilterMask {
    int number;
    int value;
};

/* Public: A CAN transceiver message filter.
 *
 * number - The ID of this filter, e.g. 0, 1, 2.
 * value - The filter's value.
 * channel - The CAN channel this filter should be applied to.
 * maskNumber - The ID of the mask this filter should be paired with.
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
 * value - The integer value of the state on the CAN bus.
 * name  - The corresponding string name for the state in OpenXC>
 */
struct CanSignalState {
    int value;
    char* name;
};

/* Public: A CAN signal to decode from the bus and output over USB.
 *
 * bus         - The CAN bus this signal belongs on.
 * messageId   - The ID of the message this signal is a part of signal.
 * genericName - The name of the signal to be output over USB.
 * bitPosition - The starting bit of the signal in its CAN message.
 * bitSize     - The width of the bit field in the CAN message.
 * factor      - The final value will be multiplied by this factor.
 * offset      - The final value will be added to this offset.
 * minValue    - The minimum value for the processed signal.
 * maxValue    - The maximum value for the processed signal.
 * sendFrequency - how often to pass along this message when received.
 * sendSame    - if true, will re-send even if the value hasn't changed.
 * received    - mark true if this signal has ever been received.
 */
struct CanSignal {
    CanBus* bus;
    uint32_t messageId;
    char* genericName;
    int bitPosition;
    int bitSize;
    float factor;
    float offset;
    float minValue;
    float maxValue;
    int sendFrequency;
    int sendClock;
    bool sendSame;
    bool received;
    CanSignalState* states;
    int stateCount;
    bool writable;
    uint64_t (*writeHandler)(CanSignal*, CanSignal*, int, cJSON*, bool*);
    float lastValue;
};

/* Public: Look up the CanSignal representation of a signal based on its generic
 *         name.
 *
 * name - The generic, OpenXC name of the signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 *
 * Returns a pointer to the CanSignal if found, otherwise null.
 */
CanSignal* lookupSignal(char* name, CanSignal* signals, int signalCount);

/* Public: Look up a CanSignalState for a CanSignal by its textual name.
 *
 * name - The generic, OpenXC name of the signal.
 * signals - The list of all signals.
 * signalCount - The length of the signals array.
 * name - the genericName of the desired signal.
 *
 * Returns
 */
CanSignalState* lookupSignalState(CanSignal* signal, CanSignal* signals,
        int signalCount, char* name);

CanSignalState* lookupSignalState(CanSignal* signal, CanSignal* signals,
        int signalCount, int value);

#endif // _CANUTIL_H_
