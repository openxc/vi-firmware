#include "serialutil.h"
#include "buffers.h"
#include "log.h"

using openxc::interface::serial::SerialDevice;

bool SERIAL_PROCESSED = false;

void openxc::interface::serial::processSerialSendQueue(SerialDevice* device) {
    SERIAL_PROCESSED = true;
}

void openxc::interface::serial::readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) { }

void openxc::interface::serial::initializeSerial(SerialDevice* serial) {
    initializeSerialCommon(serial);
}
