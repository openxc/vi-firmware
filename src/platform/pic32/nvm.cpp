#include "nvm.h"
#include "config.h"
#include "telit_he910.h"
extern "C"
{
#include "/usr/lib/mpide/hardware/pic32/libraries/EEPROM/utility/flash.h"
}

using openxc::config::getConfiguration;
using openxc::telitHE910::ModemConfigurationDescriptor;

typedef struct {
    unsigned int active;
    ModemConfigurationDescriptor config;
} _EEPROM;

static _EEPROM* eeprom = (_EEPROM*)NVM_START;

void openxc::nvm::store() {
    unsigned int i = 0;
    unsigned int writeWord;
    unsigned int* config = (unsigned int*)&(getConfiguration()->telit->config);
    eraseFlashPage((void*)NVM_START);
    writeFlashWord((void*)NVM_START+i, (unsigned int)0x00000000);
    while(i < sizeof(ModemConfigurationDescriptor)) {
        writeFlashWord((void*)NVM_START+i+4, *(unsigned int*)config);
        config++;
        i += 4;
    }
}

void openxc::nvm::load() {
    ModemConfigurationDescriptor* config = &(getConfiguration()->telit->config);
    memcpy(config, (const void*)(NVM_START+4), sizeof(ModemConfigurationDescriptor));
}

bool isActive() {
    return eeprom->active == 0;
}

void openxc::nvm::initialize() {
    if(isActive()) {
        load();
    } else {
       store();
    }
}
