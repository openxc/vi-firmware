#include "lights.h"
#include "gpio.h"
#include "led.h"

#if defined(CHIPKIT)

    #define USER_LED_A_SUPPORT
    #define USER_LED_A_POLARITY    1        // turn on LED = drive pin high
    #define USER_LED_A_PIN         13

#endif

#if defined CROSSCHASM_C5_BT || defined CROSSCHASM_C5_BLE ||  defined CROSSCHASM_C5_CELLULAR
#define CROSSCHASM_C5_COMMON
#endif

#if defined(CROSSCHASM_C5_COMMON)
	#define USER_LED_A_SUPPORT
	#define USER_LED_B_SUPPORT
	#define USER_LED_C_SUPPORT
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

#ifndef CROSSCHASM_C5_COMMON
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
#else
	switch(light)
	{
		case LIGHT_A:
			if (color.r == 0 && color.g == 0 && color.b == 0)
				led_green_off();
			else
				led_green_on();
		break;
		case LIGHT_B:
			if (color.r == 0 && color.g == 0 && color.b == 0)
				led_blue_off();
			else
				led_blue_on();
		break;
		case LIGHT_C:
			if (color.r == 0 && color.g == 0 && color.b == 0)
				led_red_off();
			else
				led_red_on();
		break;
	}
#endif

}

void openxc::lights::initialize() {
    initializeCommon();
	
#ifndef CROSSCHASM_C5_COMMON
    #if defined(USER_LED_A_SUPPORT)
    gpio::setDirection(0, USER_LED_A_PIN, GPIO_DIRECTION_OUTPUT);
    #endif

    #if defined(USER_LED_B_SUPPORT)
    gpio::setDirection(0, USER_LED_B_PIN, GPIO_DIRECTION_OUTPUT);
    #endif
    
    #if defined(USER_LED_C_SUPPORT)
    gpio::setDirection(0, USER_LED_C_PIN, GPIO_DIRECTION_OUTPUT);
    #endif
#else
	led_green_enb(1);led_green_off();
	led_red_enb(1);led_red_off();
	led_blue_enb(1);led_blue_off();
	
#endif
   
}
