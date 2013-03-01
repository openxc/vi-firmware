#include "gpio.h"

void setGpioDirection(uint32_t port, uint32_t pin, GpioDirection direction) {
    pinMode(pin, direction);
}

void setGpioValue(uint32_t port, uint32_t pin, GpioValue value) {
    digitalWrite(pin, value);
}

GpioValue getGpioValue(uint32_t port, uint32_t pin) {
    return digitalRead(pin);
}
