#include "lights.h"
#include "gpio.h"

#if defined(CROSSCHASM_C5)

    #define USER_LED_A_SUPPORT
    #define USER_LED_A_POLARITY 0 // turn on LED = drive pin low
    #define USER_LED_A_PIN 3 // PORTD BIT0 (RD0) = GREEN

    #define USER_LED_B_SUPPORT
    #define USER_LED_B_POLARITY 0 // turn on LED = drive pin low
    #define USER_LED_B_PIN 4 // PORTC BIT14 (RC14) = BLUE

#elif defined(CROSSCHASM_CELLULAR_C5)

    #define USER_LED_A_SUPPORT
    #define USER_LED_A_POLARITY 0 // turn on LED = drive pin low
    #define USER_LED_A_PIN 3 // PORTD BIT0 (RD0) = GREEN

    #define USER_LED_B_SUPPORT
    #define USER_LED_B_POLARITY 0 // turn on LED = drive pin low
    #define USER_LED_B_PIN 4 // PORTC BIT14 (RC14) = BLUE

#elif defined(CROSSCHASM_BTLE_C5)

// GREEN- RB15 - 69 TOP
// BLUE - RB12 - 67 CENTRE
// RED -  RB13 - 66 BOTTOM

	#define USER_LED_A_SUPPORT
    #define USER_LED_A_POLARITY 0
    #define USER_LED_A_PIN 66 	//RED

    #define USER_LED_B_SUPPORT
    #define USER_LED_B_POLARITY 0 
    #define USER_LED_B_PIN 67 //BLUE
	
	#define USER_LED_C_SUPPORT //shared with bootloader
    #define USER_LED_C_POLARITY 0
    #define USER_LED_C_PIN 69  //GREEN
	
#elif defined(CHIPKIT)

    #define USER_LED_A_SUPPORT
    #define USER_LED_A_POLARITY    1        // turn on LED = drive pin high
    #define USER_LED_A_PIN         13

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
	
#ifdef CROSSCHASM_BTLE_C5

	if(color.r == 255 && color.g == 0 && color.b == 0){
		gpio::setValue(0, USER_LED_A_PIN, GPIO_VALUE_LOW);
	}
	else if(color.r == 0 && color.g == 0 && color.b == 255){
		gpio::setValue(0, USER_LED_B_PIN, GPIO_VALUE_LOW);
	}
	else if(color.r == 0 && color.g == 255 && color.b == 0){
		gpio::setValue(0, USER_LED_C_PIN, GPIO_VALUE_LOW);
	}
	else
	{
		switch(light)
		{
			case LIGHT_A:
				gpio::setValue(0, USER_LED_A_PIN, GPIO_VALUE_HIGH);
			break;
			
			case LIGHT_B:
				gpio::setValue(0, USER_LED_B_PIN, GPIO_VALUE_HIGH);
			break;
			
			case LIGHT_C:
				gpio::setValue(0, USER_LED_C_PIN, GPIO_VALUE_HIGH);
			break;
		}
	}
	
#else		
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
#endif
}

void openxc::lights::initialize() {
    initializeCommon();
    #if defined(USER_LED_A_SUPPORT)
    gpio::setDirection(0, USER_LED_A_PIN, GPIO_DIRECTION_OUTPUT);
    #endif

    #if defined(USER_LED_B_SUPPORT)
    gpio::setDirection(0, USER_LED_B_PIN, GPIO_DIRECTION_OUTPUT);
    #endif
	
	#if defined(USER_LED_C_SUPPORT)
	gpio::setDirection(0, USER_LED_C_PIN, GPIO_DIRECTION_OUTPUT);
    #endif

#ifdef CROSSCHASM_BTLE_C5
	gpio::setValue(0, USER_LED_A_PIN, GPIO_VALUE_HIGH);
	gpio::setValue(0, USER_LED_B_PIN, GPIO_VALUE_HIGH);
	gpio::setValue(0, USER_LED_C_PIN, GPIO_VALUE_HIGH);
#endif	
}
