#ifndef _SIGNALS_H_
#define _SIGNALS_H_

CanSignal* getSignalList();

CanBus* getCanBuses();

void decodeCanMessage(int id, uint8_t* data);

#endif // _SIGNALS_H_
