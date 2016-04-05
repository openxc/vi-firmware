#ifndef _LED_H_
#define _LED_H_

#ifdef __cplusplus
extern "C" {
#endif
void led_red_enb(unsigned char v);
void led_red_on(void);
void led_red_off(void);

void led_blue_enb(unsigned char v);
void led_blue_on(void);
void led_blue_off(void);

void led_green_enb(unsigned char v);
void led_green_on(void);
void led_green_off(void);


#ifdef __cplusplus
}
#endif

#endif