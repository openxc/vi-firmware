#include "serialutil.h"
#include "buffers.h"
#include "log.h"

bool SERIAL_PROCESSED = false;

void processSerialSendQueue(SerialDevice* device) {
    SERIAL_PROCESSED = true;
}

void readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) { }

void initializeSerial(SerialDevice* serial) {
    initializeSerialCommon(serial);
}
