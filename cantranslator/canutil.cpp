#include "canutil.h"

void configure_hs_filters(CAN *canMod) {
    extern int FILTER_COUNT;
    extern CanFilter* FILTERS;
    extern int FILTER_MASK_COUNT;
    extern CanFilterMask* FILTER_MASKS;

    for(int i = 0; i < FILTER_MASK_COUNT; i++) {
        canMod->configureFilterMask((CAN::FILTER_MASK) FILTER_MASKS[i].number,
                FILTER_MASKS[i].value, CAN::SID, CAN::FILTER_MASK_IDE_TYPE);
    }

    for(int i = 0; i < FILTER_COUNT; i++) {
        canMod->configureFilter((CAN::FILTER) FILTERS[i].number,
                FILTERS[i].value, CAN::SID);
        canMod->linkFilterToChannel((CAN::FILTER) FILTERS[i].number,
                (CAN::FILTER_MASK) FILTERS[i].maskNumber,
                (CAN::CHANNEL) FILTERS[i].channel);
        canMod->enableFilter((CAN::FILTER) FILTERS[i].number, true);
    }
}
