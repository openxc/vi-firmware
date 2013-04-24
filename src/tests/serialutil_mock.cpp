#include "serialutil.h"
#include "buffers.h"
#include "log.h"

using openxc::serial::SerialDevice;

bool SERIAL_PROCESSED = false;

void openxc::serial::processSerialSendQueue(SerialDevice* device) {
    SERIAL_PROCESSED = true;
}

void openxc::serial::readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) { }

void openxc::serial::initializeSerial(SerialDevice* serial) {
    initializeSerialCommon(serial);
}
