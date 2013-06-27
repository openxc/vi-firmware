#include "bluetooth.h"
#include "gpio.h"
#include "util/log.h"
#include "interface/uart.h"

#ifdef CROSSCHASM_C5

    #define BLUETOOTH_SUPPORT

    #define BLUETOOTH_ENABLE_PIN_POLARITY 1 // drive high == power on
    #define BLUETOOTH_ENABLE_PORT 0
    #define BLUETOOTH_ENABLE_PIN 32 // PORTE BIT5 (RE5)

    // no other PIC32 boards supported right now have an enable pin for
    // Bluetooth - using the Sparkfun BlueSMiRF module, you can't control the
    // status via GPIO.

#endif
