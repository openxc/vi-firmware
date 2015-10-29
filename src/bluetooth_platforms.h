#if defined(FORDBOARD)

    #define BLUETOOTH_SUPPORT

    #define BLUETOOTH_ENABLE_SUPPORT

    #define BLUETOOTH_ENABLE_PIN_POLARITY 1 // drive high == power on
    #define BLUETOOTH_ENABLE_PORT 0
    #define BLUETOOTH_ENABLE_PIN 17

#elif defined(CROSSCHASM_C5_BT)

    #define BLUETOOTH_SUPPORT

    #define BLUETOOTH_ENABLE_SUPPORT
    // no other PIC32 boards supported right now have an enable pin for
    // Bluetooth - using the Sparkfun BlueSMiRF module, you can't control the
    // status via GPIO.

    #define BLUETOOTH_ENABLE_PIN_POLARITY 1
    #define BLUETOOTH_ENABLE_PORT 0
    #define BLUETOOTH_ENABLE_PIN 32 // PORTE BIT5 (RE5)
	
#elif defined(CROSSCHASM_C5_CELLULAR)

#elif defined(CHIPKIT)

    #define BLUETOOTH_SUPPORT

#endif
