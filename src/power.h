#ifndef _POWER_H_
#define _POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

void initializePower();

void updatePower();

void enterLowPowerMode();

void handleWake();

#ifdef __cplusplus
}
#endif

#endif // _POWER_H_
