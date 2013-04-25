#include "gpio.h"
#include "WProgram.h"

void openxc::gpio::setGpioDirection(uint32_t port, uint32_t pin, GpioDirection direction) {
    pinMode(pin, direction);
}

void openxc::gpio::setGpioValue(uint32_t port, uint32_t pin, GpioValue value) {
    digitalWrite(pin, value);
}

openxc::gpio::GpioValue openxc::gpio::getGpioValue(uint32_t port, uint32_t pin) {
    return digitalRead(pin) == HIGH ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;;
}
