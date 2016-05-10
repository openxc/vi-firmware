#ifdef __TEST_MODE__
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