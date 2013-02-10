#include "LPC17xx.h"
#include "PWM/pwm.h"
#include "lpc17xx_pinsel.h"
#include "light.h"

#define LED_CHANNEL 1

#define PWM_FUNCNUM 2
#define RIGHT_LED_PORT 1
#define RIGHT_LED_R_PIN 26
#define RIGHT_LED_G_PIN 23
#define RIGHT_LED_B_PIN 24
#define RIGHT_LED_R_PORT 6
#define RIGHT_LED_G_PORT 4
#define RIGHT_LED_B_PORT 5

#define LEFT_LED_PORT 1
#define LEFT_LED_R_PIN 21
#define LEFT_LED_G_PIN 20
#define LEFT_LED_B_PIN 18
#define LEFT_LED_R_PORT 3
#define LEFT_LED_G_PORT 1
#define LEFT_LED_B_PORT 2

#define MAX_PWM_VALUE 1000
#define DIMMER_DELAY 1


void disable(Light light, int duration) {
  enable(light, COLORS.black, duration);
}

void disable(Light light) {
  enable(light, COLORS.black, 0);
}

void enable(Light light, RGB color) {
}

void enable(Light light, RGB color, int duration) {
}

void flash(Light light, RGB color, int duration) {
  enable(light, color);
  // TODO make a non-blocking version of this using timers
  delayMs(duration);
  disable(light);
}

void on(int port, int up) {
    for(int i = 1; i <= MAX_PWM_VALUE; i++) {
        PWM_Set(LED_CHANNEL, port, i);
        delayMs(0, DIMMER_DELAY);
    }
}

void off(int port, int up) {
    for(int i = MAX_PWM_VALUE; i > 1; i--) {
        PWM_Set(LED_CHANNEL, port, i);
        delayMs(0, DIMMER_DELAY);
    }
}

void initializeLights() {
    PINSEL_CFG_Type PinCfg;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 1;
    PinCfg.Funcnum = PWM_FUNCNUM;
    PinCfg.Portnum = RIGHT_LED_PORT;
    PinCfg.Pinnum = RIGHT_LED_R_PIN;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = RIGHT_LED_G_PIN;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = RIGHT_LED_B_PIN;
    PINSEL_ConfigPin(&PinCfg);

    PinCfg.Pinnum = LEFT_LED_R_PIN;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = LEFT_LED_G_PIN;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = LEFT_LED_B_PIN;
    PINSEL_ConfigPin(&PinCfg);

    // Initialize all PWM controllers
    PWM_Init(LED_CHANNEL, RIGHT_LED_R_PORT, MAX_PWM_VALUE);
    PWM_Init(LED_CHANNEL, RIGHT_LED_G_PORT, MAX_PWM_VALUE);
    PWM_Init(LED_CHANNEL, RIGHT_LED_B_PORT, MAX_PWM_VALUE);
    PWM_Init(LED_CHANNEL, LEFT_LED_R_PORT, MAX_PWM_VALUE);
    PWM_Init(LED_CHANNEL, LEFT_LED_G_PORT, MAX_PWM_VALUE);
    PWM_Init(LED_CHANNEL, LEFT_LED_B_PORT, MAX_PWM_VALUE);
    PWM_Start(LED_CHANNEL);
}
