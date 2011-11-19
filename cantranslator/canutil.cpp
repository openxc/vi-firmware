#include "canutil.h"
#include "usbutil.h"

void configureFilters(CAN *can_module, CanFilterMask* filterMasks,
        CanFilter* filters) {
    extern int FILTER_COUNT;
    extern int FILTER_MASK_COUNT;

    Serial.print("Configuring ");
    Serial.print(FILTER_MASK_COUNT, DEC);
    Serial.print(" filter masks...  ");
    for(int i = 0; i < FILTER_MASK_COUNT; i++) {
        Serial.print("Configuring filter mask ");
        Serial.println(filterMasks[i].value, HEX);
        can_module->configureFilterMask(
                (CAN::FILTER_MASK) filterMasks[i].number,
                filterMasks[i].value, CAN::SID, CAN::FILTER_MASK_IDE_TYPE);
    }
    Serial.println("Done.");

    Serial.print("Configuring ");
    Serial.print(FILTER_COUNT, DEC);
    Serial.print(" filters...  ");
    for(int i = 0; i < FILTER_COUNT; i++) {
        can_module->configureFilter((CAN::FILTER) filters[i].number,
                filters[i].value, CAN::SID);
        can_module->linkFilterToChannel((CAN::FILTER) filters[i].number,
                (CAN::FILTER_MASK) filters[i].maskNumber,
                (CAN::CHANNEL) filters[i].channel);
        can_module->enableFilter((CAN::FILTER) filters[i].number, true);
    }
    Serial.println("Done.");
}

float decodeCanSignal(CanSignal* signal, uint8_t* data) {
    unsigned long rawValue = getBitField(data, signal->bitPosition,
            signal->bitSize);
    return rawValue * signal->factor + signal->offset;
}

void translateCanSignal(CanSignal* signal, uint8_t* data,
        float (*customHandler)(CanSignal*, CanSignal*, float),
        CanSignal* signals) {
    float value = decodeCanSignal(signal, data);
    value = customHandler(signal, signals, value);
    char* message = generateJson(signal, value);
    sendMessage((uint8_t*) message, strlen(message));
}

void translateCanSignal(CanSignal* signal, uint8_t* data,
        char* (*customHandler)(CanSignal*, CanSignal*, float),
        CanSignal* signals) {
    float value = decodeCanSignal(signal, data);
    char* stringValue = customHandler(signal, signals, value);
    char* message = generateJson(signal, stringValue);
    sendMessage((uint8_t*) message, strlen(message));
}

void translateCanSignal(CanSignal* signal, uint8_t* data,
        bool (*customHandler)(CanSignal*, CanSignal*, float),
        CanSignal* signals) {
    float value = decodeCanSignal(signal, data);
    bool booleanValue = customHandler(signal, signals, value);
    char* message = generateJson(signal, booleanValue);
    sendMessage((uint8_t*) message, strlen(message));
}

float passthroughHandler(CanSignal* signal, CanSignal* signals, float value) {
    return value;
}

bool booleanHandler(CanSignal* signal, CanSignal* signals, float value) {
    return value == 0.0 ? false : true;
}

char* stateHandler(CanSignal* signal, CanSignal* signals, float value) {
    for(int i = 0; i < signal->stateCount; i++) {
        if(signal->states[i].value == value) {
            return signal->states[i].name;
        }
    }
    return "";
}

void translateCanSignal(CanSignal* signal, uint8_t* data, CanSignal* signals) {
    translateCanSignal(signal, data, passthroughHandler, signals);
}

char* generateJson(CanSignal* signal, float value) {
    int messageLength = NUMERICAL_MESSAGE_FORMAT_LENGTH +
        strlen(signal->genericName) +
        NUMERICAL_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, NUMERICAL_MESSAGE_FORMAT, signal->genericName, value);
    return message;
}

char* generateJson(CanSignal* signal, char* value) {
    int messageLength = STRING_MESSAGE_FORMAT_LENGTH +
        strlen(signal->genericName) + STRING_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, STRING_MESSAGE_FORMAT, signal->genericName, value);
    return message;
}

char* generateJson(CanSignal* signal, bool value) {
    int messageLength = BOOLEAN_MESSAGE_FORMAT_LENGTH +
        strlen(signal->genericName) + BOOLEAN_MESSAGE_VALUE_MAX_LENGTH;
    char message[messageLength];
    sprintf(message, BOOLEAN_MESSAGE_FORMAT, signal->genericName,
            value ? "true" : "false");
    return message;
}
