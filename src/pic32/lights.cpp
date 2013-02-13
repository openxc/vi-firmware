#include "timer.h"
#include "lights.h"
#include "log.h"
#include "WProgram.h"

#define LED_PIN 13

void enable(Light light, RGB color) {
    switch(light) {
        case LIGHT_A:
            int status = LOW;
            if(color.r > 0 || color.g > 0 || color.b > 0) {
                status = HIGH;
            }
            digitalWrite(LED_PIN, status);
            break;
    }
}

void initializeLights() {
    pinMode(LED_PIN, OUTPUT);
}

void updateLights() {
    // TODO
}
