#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>

namespace openxc {
namespace gpio {

typedef enum {
    GPIO_DIRECTION_INPUT = 0,
    GPIO_DIRECTION_OUTPUT = 1
} GpioDirection;

typedef enum {
    GPIO_VALUE_LOW = 0,
    GPIO_VALUE_HIGH = 1
} GpioValue;

void setDirection(uint32_t port, uint32_t pin, GpioDirection direction);
void setValue(uint32_t port, uint32_t pin, GpioValue value);
GpioValue getValue(uint32_t port, uint32_t pin);

} // namespace gpio
} // namespace openxc

#endif // __GPIO_H__
