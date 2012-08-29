#ifdef __LPC17XX__

#include "serialutil.h"
#include "buffers.h"
#include "log.h"

void readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) {
    // TODO
}

void initializeSerial(SerialDevice* serial) {
    // TODO
	queue_init(&serial->receiveQueue);
}

void processInputQueue(SerialDevice* device) {
    // TODO
}

#endif // __LPC17XX__
