#include "nvm.h"

using openxc::config::getConfiguration;
using openxc::telitHE910::ModemConfigurationDescriptor;

typedef struct {
	//openxc::config::Configuration CONFIG;
	ModemConfigurationDescriptor config;
} _EEPROM;

static _EEPROM eeprom = {};

// right now we are only storing config....so just pull it directly

void EEPROM_Store() {
	unsigned int i = 0;
	unsigned int writeWord;
	eeprom.config.globalPositioningSettings.gpsEnable = true;
	//openxc::config::Configuration* config = getConfiguration();
	unsigned int* config = (unsigned int*)&(getConfiguration()->telit.config);
	eraseFlashPage((void*)NVM_START);
	while(i < sizeof(ModemConfigurationDescriptor))
	{
		// dangerous if config was right at the end of RAM and not WORD aligned
		writeFlashWord((void*)NVM_START+i, *(unsigned int*)config);
		//writeFlashWord((void*)NVM_START+i, config->globalPositioningSettings.gpsInterval);
		config++;
		i += 4;
	}
}

void EEPROM_Load() {
	ModemConfigurationDescriptor* config = &(getConfiguration()->telit.config);
	memcpy(config, (const void*)NVM_START, sizeof(ModemConfigurationDescriptor));
}
