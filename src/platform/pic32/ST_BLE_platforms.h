#if defined(CROSSCHASM_C5_BTLE)
	
	#define BLE_NO_ACT_TIMEOUT_ENABLE 1
	
	#define BLE_NO_ACT_TIMEOUT_DELAY_MS 10000
	
	#define BLE_SUPPORT
	
	#define BLE_DEBUG_STATS
	
	#define LEGACY_UART_DISABLE
	
	//#define DEBUG_LED_ON()			PORTCCLR = (1 << 13); TRISCCLR = (1 << 13)			
	
	//#define DEBUG_LED_OFF()			PORTCSET = (1 << 13); TRISCCLR = (1 << 13)
	
#endif
