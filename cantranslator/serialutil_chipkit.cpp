#include "serialutil.h"
#include "buffers.h"
#include "log.h"

void readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) {
    int bytesAvailable = serial->device->available();
    if(bytesAvailable > 0) {
        for(int i = 0; i < bytesAvailable && !queue_full(&serial->receiveQueue);
						i++) {
            char byte = serial->device->read();
			QUEUE_PUSH(uint8_t, &serial->receiveQueue, (uint8_t) byte);
        }
		processQueue(&serial->receiveQueue, callback);
    }
}

void initializeSerial(SerialDevice* serial) {
    serial->device->begin(115200);
	queue_init(&serial->receiveQueue);
}

void processInputQueue(SerialDevice* device) {
        int byteCount = 0;
        char sendBuffer[MAX_MESSAGE_SIZE];
        while(!queue_empty(&device->sendQueue) && byteCount < 64) {
            sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &device->sendQueue);
        }

        // Serial transfers are really, really slow, so we don't want to send unless
        // explicitly set to do so at compile-time. Alternatively, we could send
        // on serial whenever we detect below that we're probably not connected to a
        // USB device, but explicit is better than implicit.
        device->device->write((const uint8_t*)sendBuffer, byteCount);
}
