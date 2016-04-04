#include <plib.h>
#include "c5_common_platforms.h"
#include "led.h"

void led_red_enb(unsigned char v){
	if(v)LEDRED_ENABLE();
}

void led_red_on(void){
	LEDRED_ON();  
}

void led_red_off(void){
	LEDRED_OFF(); 
}

void led_blue_enb(unsigned char v){
	if(v)LEDBLUE_ENABLE();
}

void led_blue_on(void){
	LEDBLUE_ON();
}

void led_blue_off(void){
	LEDBLUE_OFF();
}

void led_green_enb(unsigned char v){
	if(v)LEDGREEN_ENABLE();
}

void led_green_on(void){
	LEDGREEN_ON();;
}

void led_green_off(void){
	LEDGREEN_OFF();
}
