#include "nvm.h"
#include "flash.h"
#include "config.h"
#include "telit_he910.h"

using openxc::config::getConfiguration;
using openxc::telitHE910::ModemConfigurationDescriptor;

typedef struct {
	//openxc::config::Configuration CONFIG;
	unsigned int active;
	ModemConfigurationDescriptor config;
} _EEPROM;

/*
static _EEPROM eeprom = {
	.active = 0xFFFFFFFF,
	.config = &(getConfiguration()->telit.config
};
*/

static _EEPROM* eeprom = (_EEPROM*)NVM_START;

// right now we are only storing config....so just pull it directly

void openxc::nvm::store() {
	unsigned int i = 0;
	unsigned int writeWord;
	//eeprom.config.globalPositioningSettings.gpsEnable = true;
	//openxc::config::Configuration* config = getConfiguration();
	//unsigned int* config = (unsigned int*)&(getConfiguration()->telit.config);
	unsigned int* config = (unsigned int*)&(getConfiguration()->telit.config);
	eraseFlashPage((void*)NVM_START);
	writeFlashWord((void*)NVM_START+i, (unsigned int)0x00000000);
	while(i < sizeof(ModemConfigurationDescriptor))
	{
		// dangerous if config was right at the end of RAM and not WORD aligned
		writeFlashWord((void*)NVM_START+i+4, *(unsigned int*)config);
		//writeFlashWord((void*)NVM_START+i, config->globalPositioningSettings.gpsInterval);s
		config++;
		i += 4;
	}
}

void openxc::nvm::load() {
	ModemConfigurationDescriptor* config = &(getConfiguration()->telit.config);
	memcpy(config, (const void*)(NVM_START+4), sizeof(ModemConfigurationDescriptor));
}

bool isActive() {
	return eeprom->active == 0;
}

void openxc::nvm::initialize() {
	if(isActive())
		load();
	else
		store();
}