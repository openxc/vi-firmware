#include "serialutil.h"
#include "buffers.h"

void resetBuffer(SerialDevice* serial) {
    serial->receiveBufferIndex = 0;
    memset(serial->receiveBuffer, 0, SERIAL_BUFFER_SIZE);
}

void readFromSerial(SerialDevice* serial, bool (*callback)(char*)) {
    int bytesAvailable = serial->device.available();
    if(bytesAvailable > 0) {
        for(int i = 0; i < bytesAvailable &&
                serial->receiveBufferIndex < SERIAL_BUFFER_SIZE; i++) {
            char byte = serial->device.read();
            serial->receiveBuffer[serial->receiveBufferIndex++] = byte;
        }
        processBuffer(serial->receiveBuffer, &serial->receiveBufferIndex,
                SERIAL_BUFFER_SIZE, callback);
    }
}
