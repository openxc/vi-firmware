extern void initializeVehicleInterface();
extern void firmwareLoop();

int main(void) {
    initializeVehicleInterface();
    for (;;) {
        firmwareLoop();
    }

    return 0;
}