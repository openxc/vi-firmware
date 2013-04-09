#include "canread.h"
#include <stdlib.h>
#include "log.h"

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

/* Private: Determine if the received signal should be sent out and update
 * signal metadata.
 *
 * signal - The signal to look for in the CAN message data.
 * data - The data of the CAN message.
 * send - Will be flipped to false if the signal should not be sent (e.g. the
 *      signal is on a limited send frequency and the timer is not up yet).
 *
 * Returns the float value of the signal decoded from the data.
 */
float preTranslate(CanSignal* signal, uint64_t data, bool* send) {
    float value = decodeCanSignal(signal, data);

    if(!signal->received || signal->sendClock == signal->sendFrequency - 1) {
        if(send && (!signal->received || signal->sendSame ||
                    value != signal->lastValue)) {
            signal->received = true;
        } else {
            *send = false;
        }
        signal->sendClock = 0;
    } else {
        *send = false;
        ++signal->sendClock;
    }
    return value;
}

/* Private: Update signal metadata after translating and sending.
 *
 * We keep track of the last value of each CAN signal (in its raw float form),
 * but we can't update the value until after all translation has happened,
 * in case a custom handler needs to use the value.
 */
void postTranslate(CanSignal* signal, float value) {
    signal->lastValue = value;
}

float decodeCanSignal(CanSignal* signal, uint64_t data) {
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
    debug("No signal state found for value %d", value);
    *send = false;
    return NULL;
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

void passthroughCanMessage(Listener* listener, int id, uint64_t data) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, ID_FIELD_NAME, id);

    char encodedData[67];
    union {
        uint64_t whole;
        uint8_t bytes[8];
    } combined;
    combined.whole = data;

    sprintf(encodedData, "0x%02x%02x%02x%02x%02x%02x%02x%02x",
            combined.bytes[0],
            combined.bytes[1],
            combined.bytes[2],
            combined.bytes[3],
            combined.bytes[4],
            combined.bytes[5],
            combined.bytes[6],
            combined.bytes[7]);
    cJSON_AddStringToObject(root, DATA_FIELD_NAME, encodedData);

    sendJSON(root, listener);
}

void translateCanSignal(Listener* listener, CanSignal* signal,
        uint64_t data,
        float (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    if(send) {
        float processedValue = handler(signal, signals, signalCount, value,
                &send);
        if(send) {
            sendNumericalMessage(signal->genericName, processedValue, listener);
        }
    }
    postTranslate(signal, value);
}

void translateCanSignal(Listener* listener, CanSignal* signal,
        uint64_t data,
        const char* (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    if(send) {
        const char* stringValue = handler(signal, signals, signalCount, value,
                &send);
        if(stringValue == NULL) {
            debug("No valid string returned from handler for %s",
                    signal->genericName);
        } else if(send) {
            sendStringMessage(signal->genericName, stringValue, listener);
        }
    }
    postTranslate(signal, value);
}

void translateCanSignal(Listener* listener, CanSignal* signal,
        uint64_t data,
        bool (*handler)(CanSignal*, CanSignal*, int, float, bool*),
        CanSignal* signals, int signalCount) {
    bool send = true;
    float value = preTranslate(signal, data, &send);
    if(send) {
        bool booleanValue = handler(signal, signals, signalCount, value, &send);
        if(send) {
            sendBooleanMessage(signal->genericName, booleanValue, listener);
        }
    }
    postTranslate(signal, value);
}

void translateCanSignal(Listener* listener, CanSignal* signal,
        uint64_t data, CanSignal* signals, int signalCount) {
    translateCanSignal(listener, signal, data, passthroughHandler, signals,
            signalCount);
}
