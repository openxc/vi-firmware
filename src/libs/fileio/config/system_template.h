
#include <stdint.h>
#include <stdbool.h>

// The File I/O library requires the user to define the system clock frequency (Hz)
#define SYS_CLK_FrequencySystemGet()       32000000
// The File I/O library requires the user to define the peripheral clock frequency (Hz)
#define SYS_CLK_FrequencyPeripheralGet()       SYS_CLK_FrequencySystemGet()
// The File I/O library requires the user to define the instruction clock frequency (Hz)
#define SYS_CLK_FrequencyInstructionGet()      (SYS_CLK_FrequencySystemGet() / 2)


// System initialization function
void SYSTEM_Initialize (void);

// User-defined function to set the chip select for our example drive
void USER_SdSpiSetCs_1 (uint8_t a);
// User-defined function to get the card detection status for our example drive
uint8_t USER_SdSpiGetCd_1 (void);
// User-defined function to get the write-protect status for our example drive
uint8_t USER_SdSpiGetWp_1 (void);
// User-defined function to initialize tristate bits for CS, CD, and WP
void USER_SdSpiConfigurePins_1 (void);

