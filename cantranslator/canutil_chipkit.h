#ifndef _CANUTIL_CHIPKIT_H_
#define _CANUTIL_CHIPKIT_H_

#include "canutil.h"
#include "chipKITCAN.h"
#include "usbutil.h"
#include "cJSON.h"

#define SYS_FREQ (80000000L)

/* Network Node Addresses */
#define CAN_1_ADDRESS 0x101
#define CAN_2_ADDRESS 0x102

extern CanFilterMask* initializeFilterMasks(uint32_t, int*);
extern CanFilter* initializeFilters(uint32_t, int*);

// TODO document these
void sendNumericalMessage(char* name, float value, CanUsbDevice* usbDevice);
void sendStringMessage(char* name, char* value, CanUsbDevice* usbDevice);
void sendBooleanMessage(char* name, bool value, CanUsbDevice* usbDevice);
void sendEventedBooleanMessage(char* name, char* value, bool event,
        CanUsbDevice* usbDevice);

/* Public: Initializes message filter masks and filters on the CAN controller.
 *
 * canMod - a pointer to an initialized CAN module class.
 * filterMasks - an array of the filter masks to initialize.
 * filters - an array of filters to initialize.
 */
void configureFilters(CAN *canMod, CanFilterMask* filterMasks,
        int filterMaskCount, CanFilter* filters, int filterCount);

/* Public: Parses a CAN signal from a CAN message, applies required
 *         transforations and sends the result over USB.
 *
 * usbDevice - the USB device to send the final formatted message on.
 * signal - the details of the signal to decode and forward.
 * data   - the raw bytes of the CAN message that contains the signal.
 */
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal, uint8_t* data,
        CanSignal* signals, int signalCount);

/* Public: Parses a CAN signal from a CAN message, applies required
 *         transforations and also runs the final float value through the
 *         handler function before sending the result out over USB.
 *
 * usbDevice - the USB device to send the final formatted message on.
 * signal - the details of the signal to decode and forward.
 * data - the raw bytes of the CAN message that contains the signal.
 * handler - a function pointer that performs extra processing on the
 *           float value.
 * signals - an array of all active signals.
 * signalCount - the length of the signals array
 */
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount);

/* Initialize the CAN controller. See inline comments for description of the
 * process.
 */
void initializeCan(CAN* bus, int address, int speed, uint8_t* messageArea);

void sendCanSignal(CAN* bus, CanSignal* signal,
        cJSON* value,
        uint32_t (*writer)(CanSignal*, CanSignal*, int, cJSON*, bool*),
        CanSignal* signals, int signalCount);

#endif // _CANUTIL_CHIPKIT_H_
