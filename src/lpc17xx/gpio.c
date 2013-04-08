#include "gpio.h"
#include "LPC17xx.h"

static LPC_GPIO_TypeDef* const LPC_GPIO[5] = {
    LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3, LPC_GPIO4
};

void setGpioDirection(uint32_t port, uint32_t pin, GpioDirection direction) {
    uint32_t bitmask = 1 << pin;
    if(direction == GPIO_DIRECTION_OUTPUT) {
        LPC_GPIO[port]->FIODIR |= bitmask;
    } else {
        LPC_GPIO[port]->FIODIR &= ~bitmask;
    }
}

void setGpioValue(uint32_t port, uint32_t pin, GpioValue value) {
    uint32_t bitmask = 1 << pin;
    if(value == GPIO_VALUE_LOW) {
        // FIOCLR is faster than FIOSET to 0
        LPC_GPIO[port]->FIOCLR = bitmask;
    } else {
        LPC_GPIO[port]->FIOSET = bitmask;
    }
}

GpioValue getGpioValue(uint32_t port, uint32_t pin) {
    LPC_GPIO[port]->FIOMASK = ~(1 << pin);
    GpioValue value = LPC_GPIO[port]->FIOPIN >> pin;
    LPC_GPIO[port]->FIOMASK = 0;
    return value;
}
