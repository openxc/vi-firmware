#include "serialutil.h"
#include "buffers.h"

void readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) {
    int bytesAvailable = serial->device->available();
    if(bytesAvailable > 0) {
        for(int i = 0; i < bytesAvailable && !queue_full(&serial->receiveQueue);
						i++) {
            char byte = serial->device->read();
			queue_push(&serial->receiveQueue, (uint8_t) byte);
        }
		processQueue(&serial->receiveQueue, callback);
    }
}

void initializeSerial(SerialDevice* serial) {
    serial->device->begin(115200);
	queue_init(&serial->receiveQueue);
}
