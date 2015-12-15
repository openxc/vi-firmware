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
#else
extern void initializeTestInterface();
extern void testfirmwareLoop();

	int main(void) {
    initializeTestInterface();
    for (;;) {
        testfirmwareLoop();
    }
    return 0;
}	
#endif