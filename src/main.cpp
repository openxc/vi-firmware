extern void initializeVehicleInterface();
extern void firmareLoop();

int main(void) {

    initializeVehicleInterface();
    for (;;) {
        firmareLoop();
    }

    return 0;
}

