#include "gpio.h"
#include "WProgram.h"

void openxc::gpio::setDirection(uint32_t port, uint32_t pin, GpioDirection direction) {
    pinMode(pin, direction);
}

void openxc::gpio::setValue(uint32_t port, uint32_t pin, GpioValue value) {
    digitalWrite(pin, value);
}

openxc::gpio::GpioValue openxc::gpio::getValue(uint32_t port, uint32_t pin) {
    return digitalRead(pin) == HIGH ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;;
}
