#include "interface/uart.h"
#include "util/bytebuffer.h"
#include "util/log.h"

using openxc::interface::uart::SerialDevice;

bool SERIAL_PROCESSED = false;

void openxc::interface::uart::processSerialSendQueue(SerialDevice* device) {
    SERIAL_PROCESSED = true;
}

void openxc::interface::uart::readFromSerial(SerialDevice* serial, bool (*callback)(uint8_t*)) { }

void openxc::interface::uart::initializeSerial(SerialDevice* serial) {
    initializeSerialCommon(serial);
}
