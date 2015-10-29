#include "lights.h"
#include "gpio.h"

#if defined(CROSSCHASM_C5_BT)

    #define USER_LED_A_SUPPORT
    #define USER_LED_A_POLARITY 0 // turn on LED = drive pin low
    #define USER_LED_A_PIN 3 // PORTD BIT0 (RD0) = GREEN

    #define USER_LED_B_SUPPORT
    #define USER_LED_B_POLARITY 0 // turn on LED = drive pin low
    #define USER_LED_B_PIN 4 // PORTC BIT14 (RC14) = BLUE

#elif defined(CROSSCHASM_C5_CELLULAR)

    #define USER_LED_A_SUPPORT
    #define USER_LED_A_POLARITY 0 // turn on LED = drive pin low
    #define USER_LED_A_PIN 3 // PORTD BIT0 (RD0) = GREEN

    #define USER_LED_B_SUPPORT
    #define USER_LED_B_POLARITY 0 // turn on LED = drive pin low
    #define USER_LED_B_PIN 4 // PORTC BIT14 (RC14) = BLUE

#elif defined(CHIPKIT)

    #define USER_LED_A_SUPPORT
    #define USER_LED_A_POLARITY    1        // turn on LED = drive pin high
    #define USER_LED_A_PIN         13

#endif

namespace gpio = openxc::gpio;

using openxc::gpio::GpioValue;
using openxc::gpio::GPIO_VALUE_LOW;
using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_DIRECTION_OUTPUT;

void enablePin(openxc::lights::RGB color, int pin, int polarity) {
    GpioValue value;
    if(color.r == 0 && color.g == 0 && color.b == 0) {
        value = polarity ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;
    } else {
        value = polarity ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
    }
    gpio::setValue(0, pin, value);
}

void openxc::lights::enable(Light light, RGB color) {
    switch(light) {
#if defined(USER_LED_A_SUPPORT)
        case LIGHT_A:
            enablePin(color, USER_LED_A_PIN, USER_LED_A_POLARITY);
            break;
#endif

#if defined(USER_LED_B_SUPPORT)
        case LIGHT_B:
            enablePin(color, USER_LED_B_PIN, USER_LED_B_POLARITY);
            break;
#endif
        default:
            break;
    }
}

void openxc::lights::initialize() {
    initializeCommon();
    #if defined(USER_LED_A_SUPPORT)
    gpio::setDirection(0, USER_LED_A_PIN, GPIO_DIRECTION_OUTPUT);
    #endif

    #if defined(USER_LED_B_SUPPORT)
    gpio::setDirection(0, USER_LED_B_PIN, GPIO_DIRECTION_OUTPUT);
    #endif
}
