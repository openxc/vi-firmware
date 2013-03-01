#include "lights.h"
#include "gpio.h"

#define USER_LED_PIN 13

void enable(Light light, RGB color) {
    GpioValue value;
    if(color.r == 0 && color.g == 0 && color.b == 0) {
        value = GPIO_VALUE_LOW;
    } else {
        value = GPIO_VALUE_HIGH;
    }

    switch(light) {
        case LIGHT_A:
            setGpioValue(0, USER_LED_PIN, value);
            break;
    }
}

void initializeLights() {
    setGpioDirection(0, USER_LED_PIN, GPIO_DIRECTION_OUTPUT);
}
