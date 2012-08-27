#include "canread.h"
#include <stdlib.h>

float decodeCanSignal(CanSignal* signal, uint8_t* data) {
    uint64_t rawValue = getBitField(data, signal->bitPosition,
            signal->bitSize);
    return rawValue * signal->factor + signal->offset;
}

float passthroughHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return value;
}

bool booleanHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    return value == 0.0 ? false : true;
}

float ignoreHandler(CanSignal* signal, CanSignal* signals, int signalCount,
        float value, bool* send) {
    *send = false;
    return value;
}

const char* stateHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, float value, bool* send) {
    CanSignalState* signalState = lookupSignalState(value, signal, signals,
            signalCount);
    if(signalState != NULL) {
        return signalState->name;
    }
    *send = false;
    return NULL;
}

/* Private: Serialize the root JSON object to a string (ending with a newline)
 * and send it to the listener.
 *
 * root - The JSON object to send.
 * listener - The listener device to send on.
 */
void sendJSON(cJSON* root, Listener* listener) {
    char* message = cJSON_PrintUnformatted(root);
    sendMessage(listener, (uint8_t*) message, strlen(message));
    cJSON_Delete(root);
    free(message);
}

/* Private: Combine the given name and value into a JSON object (conforming to
 * the OpenXC standard) and send it out to the listener.
 *
 * name - The value for the name field of the OpenXC message.
 * value - The numerical, string or booelan for the value field of the OpenXC
 *     message.
 * event - (Optional) The event for the event field of the OpenXC message.
 * listener - The listener device to send on.
 */
void sendJSONMessage(const char* name, cJSON* value, cJSON* event,
        Listener* listener) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, NAME_FIELD_NAME, name);
    cJSON_AddItemToObject(root, VALUE_FIELD_NAME, value);
    if(event != NULL) {
        cJSON_AddItemToObject(root, EVENT_FIELD_NAME, event);
    }
    sendJSON(root, listener);
}

void sendNumericalMessage(const char* name, float value, Listener* listener) {
    sendJSONMessage(name, cJSON_CreateNumber(value), NULL, listener);
}

void sendBooleanMessage(const char* name, bool value, Listener* listener) {
    sendJSONMessage(name, cJSON_CreateBool(value), NULL, listener);
}

void sendStringMessage(const char* name, const char* value,
        Listener* listener) {
    sendJSONMessage(name, cJSON_CreateString(value), NULL, listener);
}

void sendEventedBooleanMessage(const char* name, const char* value, bool event,
        Listener* listener) {
    sendJSONMessage(name, cJSON_CreateString(value), cJSON_CreateBool(event),
            listener);
}

void sendEventedStringMessage(const char* name, const char* value,
        const char* event, Listener* listener) {
    sendJSONMessage(name, cJSON_CreateString(value), cJSON_CreateString(event),
            listener);
}

// TODO there is lots of duplicated code in these functions, but I don't see an
// obvious way to share code and still keep the different data types returned
// by the handlers.
void translateCanSignal(Listener* listener, CanSignal* signal,
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
            sendNumericalMessage(signal->genericName, processedValue, listener);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void translateCanSignal(Listener* listener, CanSignal* signal,
        uint8_t* data,
        const char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    float value = decodeCanSignal(signal, data);
    bool send = true;
    const char* stringValue = handler(signal, signals, signalCount, value,
            &send);

    if(signal->sendClock == signal->sendFrequency) {
        if(send && (signal->sendSame || !signal->received ||
                    value != signal->lastValue)) {
            signal->received = true;
            sendStringMessage(signal->genericName, stringValue, listener);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void translateCanSignal(Listener* listener, CanSignal* signal,
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
            sendBooleanMessage(signal->genericName, booleanValue, listener);
        }
        signal->sendClock = 0;
    } else {
        ++signal->sendClock;
    }
    signal->lastValue = value;
}

void translateCanSignal(Listener* listener, CanSignal* signal,
        uint8_t* data, CanSignal* signals, int signalCount) {
    translateCanSignal(listener, signal, data, passthroughHandler, signals,
            signalCount);
}
