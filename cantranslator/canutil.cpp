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

void translateCanSignal(CanSignal* signal, uint8_t* data) {
    float value = decodeCanSignal(signal, data);
    char* message = generateJson(signal, value);
    // TODO what do we need to include to use strnlen here? we know the max
    // length
    sendMessage((uint8_t*) message, strlen(message));
}

char* generateJson(CanSignal* signal, float value) {
    int message_length = MESSAGE_FORMAT_LENGTH + strlen(signal->genericName) +
        MESSAGE_VALUE_MAX_LENGTH;
    char message[message_length];
    sprintf(message, MESSAGE_FORMAT, signal->genericName, value);
    return message;
}
