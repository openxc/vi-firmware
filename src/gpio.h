#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GPIO_DIRECTION_OUTPUT = 0,
    GPIO_DIRECTION_INPUT = 1
} GpioDirection;

typedef enum {
    GPIO_VALUE_LOW = 0,
    GPIO_VALUE_HIGH = 1
} GpioValue;

void setGpioDirection(uint32_t port, uint32_t pin, GpioDirection direction);
void setGpioValue(uint32_t port, uint32_t pin, GpioValue value);
GpioValue getGpioValue(uint32_t port, uint32_t pin);

#ifdef __cplusplus
}
#endif

#endif // __GPIO_H__
