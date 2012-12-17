#include "serialutil.h"
#include "buffers.h"
#include "log.h"
#include "WProgram.h"

#define UART_BAUDRATE 115200

// TODO see if we can do this with interrupts on the chipKIT
// http://www.chipkit.org/forum/viewtopic.php?f=7&t=1088
void readFromSerial(SerialDevice* device, bool (*callback)(uint8_t*)) {
    int bytesAvailable = ((HardwareSerial*)device->controller)->available();
    if(bytesAvailable > 0) {
        for(int i = 0; i < bytesAvailable && !QUEUE_FULL(uint8_t,
								&device->receiveQueue); i++) {
            char byte = ((HardwareSerial*)device->controller)->read();
            QUEUE_PUSH(uint8_t, &device->receiveQueue, (uint8_t) byte);
        }
        processQueue(&device->receiveQueue, callback);
    }
}

void initializeSerial(SerialDevice* device) {
	device->controller = &Serial1;
    ((HardwareSerial*)device->controller)->begin(UART_BAUDRATE);
    QUEUE_INIT(uint8_t, &device->receiveQueue);
    QUEUE_INIT(uint8_t, &device->sendQueue);
}

// The chipKIT version of this function is blocking. It will entirely flush the
// send queue before returning.
void processSerialSendQueue(SerialDevice* device) {
    int byteCount = 0;
    char sendBuffer[MAX_MESSAGE_SIZE];
    while(!QUEUE_EMPTY(uint8_t, &device->sendQueue) &&
					byteCount < MAX_MESSAGE_SIZE) {
        sendBuffer[byteCount++] = QUEUE_POP(uint8_t, &device->sendQueue);
    }

    ((HardwareSerial*)device->controller)->write((const uint8_t*)sendBuffer,
			byteCount);
}
