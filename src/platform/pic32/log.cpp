#include "util/log.h"
#include <stdio.h>
#include <stdarg.h>
#include "WProgram.h"
#include "interface/uart.h"
extern HardwareSerial Serial2;

extern HardwareSerial Serial;

namespace uart = openxc::interface::uart;

void openxc::util::log::initialize() {    

#if defined(CROSSCHASM_C5_BLE)
        Serial.begin(115200);
#elif defined(CROSSCHASM_C5_BT)
        return; //Serial.begin(115200);
#elif defined(CROSSCHASM_C5_CELLULAR)
        Serial.begin(115200);
#else 
        Serial2.begin(115200);
#endif
}

void openxc::util::log::debugUart(const char* message) {

#if defined(CROSSCHASM_C5_BLE)
        Serial.print(message);
#elif defined(CROSSCHASM_C5_BT)
        return;//Serial.print(message);
#elif defined(CROSSCHASM_C5_CELLULAR)
        Serial.print(message);
#else 
        Serial2.print(message);
#endif
}
