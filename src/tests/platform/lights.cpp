#include "lights.h"

openxc::lights::RGB LIGHT_A_LAST_COLOR;
openxc::lights::RGB LIGHT_B_LAST_COLOR;

void openxc::lights::enable(Light light, RGB color) {
    if(light == LIGHT_A) {
        LIGHT_A_LAST_COLOR = color;
    } else {
        LIGHT_B_LAST_COLOR = color;
    }
}

void openxc::lights::initialize() {
    initializeCommon();
}
