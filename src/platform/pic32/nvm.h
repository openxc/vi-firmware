#ifndef _NVM_H_
#define _NVM_H_

#include "flash.h"
#include "config.h"

#define NVM_START	0x9D07F000
#define NVM_SIZE	0x1000

void EEPROM_Store();
void EEPROM_Load();

#endif