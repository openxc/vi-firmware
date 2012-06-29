#include "serialutil.h"

void readFromSerial(SerialDevice* serial, bool (*callback)(char*)) {
    int bytesAvailable = serial->device.available();
    if(bytesAvailable > 0) {
        for(int i = 0; i < bytesAvailable &&
                serial->receiveBufferIndex < SERIAL_BUFFER_SIZE; i++) {
            char byte = serial->device.read();
            serial->receiveBuffer[serial->receiveBufferIndex++] = byte;
        }
    }
}
