#include "canread_chipkit.h"
#include "canread.h"
#include "WProgram.h"

/* Private: Serialize the root JSON object to a string (ending with a newline)
 * and send it over the default input endpoint for the USB device.
 *
 * root - The JSON object to send.
 * usbDevice - The USB device to send on.
 */
void sendJSON(cJSON* root, CanUsbDevice* usbDevice) {
    char *message = cJSON_PrintUnformatted(root);
    sendMessage(usbDevice, (uint8_t*) message, strlen(message));
    cJSON_Delete(root);
    free(message);
}

/* Private: Combine the given name and value into a JSON object (conforming to
 * the OpenXC standard) and send it out over the default input endpoint for the
 * USB device.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The numerical, string or booelan for the value field of the OpenXC
 *     message.
 * event - (Optional) The event for the event field of the OpenXC message.
 * usbDevice - The USB device to send on.
 */
void sendJSONMessage(char* name, cJSON* value, cJSON* event,
        CanUsbDevice* usbDevice) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, NAME_FIELD_NAME, name);
    cJSON_AddItemToObject(root, VALUE_FIELD_NAME, value);
    if(event != NULL) {
        cJSON_AddItemToObject(root, EVENT_FIELD_NAME, event);
    }
    sendJSON(root, usbDevice);
}

void sendNumericalMessage(char* name, float value, CanUsbDevice* usbDevice) {
    sendJSONMessage(name, cJSON_CreateNumber(value), NULL, usbDevice);
}

void sendBooleanMessage(char* name, bool value, CanUsbDevice* usbDevice) {
    sendJSONMessage(name, cJSON_CreateBool(value), NULL, usbDevice);
}

void sendStringMessage(char* name, char* value, CanUsbDevice* usbDevice) {
    sendJSONMessage(name, cJSON_CreateString(value), NULL, usbDevice);
}

void sendEventedBooleanMessage(char* name, char* value, bool event,
        CanUsbDevice* usbDevice) {
    sendJSONMessage(name, cJSON_CreateString(value), cJSON_CreateBool(event),
            usbDevice);
}

void sendEventedStringMessage(char* name, char* value, char* event,
        CanUsbDevice* usbDevice) {
    sendJSONMessage(name, cJSON_CreateString(value), cJSON_CreateString(event),
            usbDevice);
}

// TODO there is lots of duplicated code in these functions, but I don't see an
// obvious way to share code and still keep the different data types returned
// by the handlers.
void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    float processedValue = handler(signal, signals, signalCount, value, &send);

    if(signal->sendClock == signal->sendFrequency) {
        if(send && (signal->sendSame || !signal->received ||
                    value != signal->lastValue)) {
            signal->received = true;
            sendNumericalMessage(signal->genericName, processedValue,
                    usbDevice);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    char* stringValue = handler(signal, signals, signalCount, value, &send);

    if(signal->sendClock == signal->sendFrequency) {
        if(send && (signal->sendSame || !signal->received ||
                    value != signal->lastValue)) {
            signal->received = true;
            sendStringMessage(signal->genericName, stringValue, usbDevice);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    bool booleanValue = handler(signal, signals, signalCount, value, &send);

    if(signal->sendClock == signal->sendFrequency) {
        if(send && (signal->sendSame || !signal->received ||
                    value != signal->lastValue)) {
            signal->received = true;
            sendBooleanMessage(signal->genericName, booleanValue, usbDevice);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void translateCanSignal(CanUsbDevice* usbDevice, CanSignal* signal,
        uint8_t* data, CanSignal* signals, int signalCount) {
    translateCanSignal(usbDevice, signal, data, passthroughHandler, signals,
            signalCount);
}
