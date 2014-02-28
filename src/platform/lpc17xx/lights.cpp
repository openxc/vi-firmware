#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_pwm.h"
#include "util/timer.h"
#include "lights.h"
#include "util/log.h"

#define LED_PWM_PERIPHERAL LPC_PWM1

#define PWM_FUNCNUM 2
#define RIGHT_LED_PORT 1
#define RIGHT_LED_R_PIN 26
#define RIGHT_LED_G_PIN 23
#define RIGHT_LED_B_PIN 24
#define RIGHT_LED_R_CHANNEL 6
#define RIGHT_LED_G_CHANNEL 4
#define RIGHT_LED_B_CHANNEL 5

#define LEFT_LED_PORT 1
#define LEFT_LED_R_PIN 21
#define LEFT_LED_G_PIN 20
#define LEFT_LED_B_PIN 18
#define LEFT_LED_R_CHANNEL 3
#define LEFT_LED_G_CHANNEL 1
#define LEFT_LED_B_CHANNEL 2

#define PWM_PERIOD_MICROSECONDS 1000
#define DIMMER_DELAY 1

void setPwm(LPC_PWM_TypeDef* pwm, int channel, int value) {
    PWM_MatchUpdate(pwm, channel, (value / 255) * PWM_PERIOD_MICROSECONDS,
            PWM_MATCH_UPDATE_NEXT_RST);
}

void openxc::lights::enable(Light light, RGB color) {
    int redChannel;
    int greenChannel;
    int blueChannel;

    switch(light) {
        case LIGHT_A:
            redChannel = RIGHT_LED_R_CHANNEL;
            greenChannel = RIGHT_LED_G_CHANNEL;
            blueChannel = RIGHT_LED_B_CHANNEL;
            break;
        case LIGHT_B:
            redChannel = LEFT_LED_R_CHANNEL;
            greenChannel = LEFT_LED_G_CHANNEL;
            blueChannel = LEFT_LED_B_CHANNEL;
            break;
        default:
            return;
    }

    setPwm(LED_PWM_PERIPHERAL, redChannel, color.r);
    setPwm(LED_PWM_PERIPHERAL, greenChannel, color.g);
    setPwm(LED_PWM_PERIPHERAL, blueChannel, color.b);
}

void configureChannel(LPC_PWM_TypeDef* pwm, int channel) {
    /* Configure match option */
    PWM_MATCHCFG_Type pwmMatchConfig;
    pwmMatchConfig.IntOnMatch = DISABLE;
    pwmMatchConfig.MatchChannel = RIGHT_LED_R_CHANNEL;
    pwmMatchConfig.ResetOnMatch = DISABLE;
    pwmMatchConfig.StopOnMatch = DISABLE;

    PWM_ConfigMatch(pwm, &pwmMatchConfig);
    /* Enable PWM Channel Output */
    PWM_ChannelCmd(pwm, channel, ENABLE);
}

void configureLightPins() {
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

    PinCfg.Portnum = LEFT_LED_PORT;
    PinCfg.Pinnum = LEFT_LED_R_PIN;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = LEFT_LED_G_PIN;
    PINSEL_ConfigPin(&PinCfg);
    PinCfg.Pinnum = LEFT_LED_B_PIN;
    PINSEL_ConfigPin(&PinCfg);
}

void initializePwm() {
    PWM_TIMERCFG_Type pwmConfig;
    PWM_ConfigStructInit(PWM_MODE_TIMER, &pwmConfig);
    PWM_Init(LED_PWM_PERIPHERAL, PWM_MODE_TIMER, &pwmConfig);
}

void setPwmPeriod(int period) {
    PWM_MatchUpdate(LED_PWM_PERIPHERAL, 0, period, PWM_MATCH_UPDATE_NOW);

    PWM_MATCHCFG_Type pwmMatchConfig;
    pwmMatchConfig.IntOnMatch = DISABLE;
    pwmMatchConfig.MatchChannel = 0;
    pwmMatchConfig.ResetOnMatch = ENABLE;
    pwmMatchConfig.StopOnMatch = DISABLE;
    PWM_ConfigMatch(LED_PWM_PERIPHERAL, &pwmMatchConfig);
}

void openxc::lights::initialize() {
    initializeCommon();
    configureLightPins();
    initializePwm();
    setPwmPeriod(PWM_PERIOD_MICROSECONDS);

    // Initialize all PWM controllers
    PWM_ChannelConfig(LED_PWM_PERIPHERAL,
            RIGHT_LED_R_CHANNEL, PWM_CHANNEL_SINGLE_EDGE);
    PWM_ChannelConfig(LED_PWM_PERIPHERAL,
            RIGHT_LED_G_CHANNEL, PWM_CHANNEL_SINGLE_EDGE);
    PWM_ChannelConfig(LED_PWM_PERIPHERAL,
            RIGHT_LED_B_CHANNEL, PWM_CHANNEL_SINGLE_EDGE);
    PWM_ChannelConfig(LED_PWM_PERIPHERAL,
            LEFT_LED_R_CHANNEL, PWM_CHANNEL_SINGLE_EDGE);
    PWM_ChannelConfig(LED_PWM_PERIPHERAL,
            LEFT_LED_G_CHANNEL, PWM_CHANNEL_SINGLE_EDGE);
    PWM_ChannelConfig(LED_PWM_PERIPHERAL,
            LEFT_LED_R_CHANNEL, PWM_CHANNEL_SINGLE_EDGE);

    configureChannel(LED_PWM_PERIPHERAL, RIGHT_LED_R_CHANNEL);
    configureChannel(LED_PWM_PERIPHERAL, RIGHT_LED_G_CHANNEL);
    configureChannel(LED_PWM_PERIPHERAL, RIGHT_LED_B_CHANNEL);
    configureChannel(LED_PWM_PERIPHERAL, LEFT_LED_R_CHANNEL);
    configureChannel(LED_PWM_PERIPHERAL, LEFT_LED_G_CHANNEL);
    configureChannel(LED_PWM_PERIPHERAL, LEFT_LED_B_CHANNEL);

    PWM_ResetCounter(LED_PWM_PERIPHERAL);
    PWM_CounterCmd(LED_PWM_PERIPHERAL, ENABLE);
    PWM_Cmd(LED_PWM_PERIPHERAL, ENABLE);
}
