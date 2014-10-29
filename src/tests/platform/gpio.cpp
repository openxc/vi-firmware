#include "gpio.h"

void openxc::gpio::setDirection(uint32_t port, uint32_t pin, GpioDirection direction) { }

void openxc::gpio::setValue(uint32_t port, uint32_t pin, GpioValue value) { }

openxc::gpio::GpioValue openxc::gpio::getValue(uint32_t port, uint32_t pin) {
    return GpioValue::GPIO_VALUE_LOW;
}
