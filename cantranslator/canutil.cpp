#include "canutil.h"
#include "usbutil.h"

void configure_hs_filters(CAN *can_module) {
    extern int FILTER_COUNT;
    extern CanFilter* FILTERS;
    extern int FILTER_MASK_COUNT;
    extern CanFilterMask* FILTER_MASKS;

    for(int i = 0; i < FILTER_MASK_COUNT; i++) {
        can_module->configureFilterMask(
                (CAN::FILTER_MASK) FILTER_MASKS[i].number,
                FILTER_MASKS[i].value, CAN::SID, CAN::FILTER_MASK_IDE_TYPE);
    }

    for(int i = 0; i < FILTER_COUNT; i++) {
        can_module->configureFilter((CAN::FILTER) FILTERS[i].number,
                FILTERS[i].value, CAN::SID);
        can_module->linkFilterToChannel((CAN::FILTER) FILTERS[i].number,
                (CAN::FILTER_MASK) FILTERS[i].maskNumber,
                (CAN::CHANNEL) FILTERS[i].channel);
        can_module->enableFilter((CAN::FILTER) FILTERS[i].number, true);
    }
}

void decode_can_signal(uint8_t* data, CanSignal* signal) {
    unsigned long raw_value;
    float final_value;

    raw_value = getBitField(data, signal->bitPosition, signal->bitSize);
    final_value = (float)raw_value * signal->factor + signal->offset;

    send_signal(signal, final_value);
}

void send_signal(CanSignal* signal, float value) {
    int message_length = MESSAGE_FORMAT_LENGTH + strlen(signal->genericName) +
        MESSAGE_VALUE_MAX_LENGTH;
    char message[message_length];
    sprintf(message, MESSAGE_FORMAT, signal->genericName, value);
    // TODO what do we need to include to use strnlen here? we know the max
    // length
    send_message((uint8_t*) message, strlen(message));
}
