#include "canread_chipkit.h"
#include "canread.h"
#include "WProgram.h"

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

void sendJSON(cJSON* root, CanUsbDevice* usbDevice) {
    char *message = cJSON_PrintUnformatted(root);
    sendMessage(usbDevice, (uint8_t*) message, strlen(message));
    cJSON_Delete(root);
    free(message);
}

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

// TODO if we make value a void*, could this be used for all of the
// translateCanSignal functions? We can pass in the formats no problem, but the
// differently typed values mean you would need to repeat this entire function
// multiple files anyway. I'm leaving this function for now because it's useful
// in a custom handler.
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
