extern void initializeVehicleInterface();
extern void firmwareLoop();

#ifndef __TEST_MODE__
int main(void) {
    initializeVehicleInterface();
    for (;;) {
        firmwareLoop();
    }

    return 0;
}
#endif