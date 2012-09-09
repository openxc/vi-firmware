#ifdef CHIPKIT

#include "serialutil.h"
#include "buffers.h"
#include "log.h"

// TODO see if we can do this with interrupts on the chipKIT
// http://www.chipkit.org/forum/viewtopic.php?f=7&t=1088
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
    while(!queue_empty(&device->sendQueue) && byteCount < MAX_MESSAGE_SIZE) {
        sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &device->sendQueue);
    }

    device->device->write((const uint8_t*)sendBuffer, byteCount);
}

#endif // CHIPKIT
