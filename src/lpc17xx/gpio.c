#include "gpio.h"
#include "LPC17xx.h"

static LPC_GPIO_TypeDef* const LPC_GPIO[5] = {
    LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3, LPC_GPIO4
};

void setGpioDirection(uint32_t port, uint32_t pin, GpioDirection direction) {
    if(direction == GPIO_DIRECTION_OUTPUT) {
        LPC_GPIO[port]->FIODIR |= 1 << pin;
    } else {
        LPC_GPIO[port]->FIODIR &= ~(1 << pin);
    }
}

void setGpioValue(uint32_t port, uint32_t pin, GpioValue value) {
    if(value == GPIO_VALUE_LOW) {
        // FIOCLR is faster than FIOSET to 0
        LPC_GPIO[port]->FIOCLR = (1 << pin);
    } else {
        LPC_GPIO[port]->FIOSET = (1 << pin);
    }
}

GpioValue getGpioValue(uint32_t port, uint32_t pin) {
    LPC_GPIO[port]->FIOMASK = ~(1 << pin);
    GpioValue value = LPC_GPIO[port]->FIOPIN >> pin;
    LPC_GPIO[port]->FIOMASK = 0;
    return value;
}
