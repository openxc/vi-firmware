#include "flash.h"

#define NVM_START	0x9D07F000
#define NVM_SIZE	0x1000

typedef struct {
	bool upgradePending;
	char upgradeFileMD5[32];
	unsigned int* flastStart;
	unsigned int flashLen;
} UpgradeInfo;