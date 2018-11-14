#ifndef __C5_COMMON_PLATFORMS_H_
#define __C5_COMMON_PLATFORMS_H_


#if defined CROSSCHASM_C5_BT || defined CROSSCHASM_C5_BLE ||  defined CROSSCHASM_C5_CELLULAR
    #define RTC_SUPPORT
	#define CROSSCHASM_C5_COMMON
	//LED
	
	#if defined CROSSCHASM_C5_BT || defined CROSSCHASM_C5_CELLULAR
	
		#define LEDRED_ENABLE()     TRISCCLR = (1 << 13)
		#define LEDRED_ON()       	LATCCLR  = (1 << 13)
		#define LEDRED_OFF()        LATCSET  = (1 << 13)
		
		#define LEDGREEN_ENABLE()   TRISDCLR = (1 << 0)
		#define LEDGREEN_ON()      	LATDCLR  = (1 << 0)
		#define LEDGREEN_OFF()      LATDSET  = (1 << 0)
		
		#define LEDBLUE_ENABLE()    TRISCCLR = (1 << 14)
		#define LEDBLUE_ON()       	LATCCLR  = (1 << 14)
		#define LEDBLUE_OFF()       LATCSET  = (1 << 14)

	
		#elif defined CROSSCHASM_C5_BLE

		#define LEDRED_ENABLE()     TRISBCLR = (1 << 13)
		#define LEDRED_ON()       	LATBCLR  = (1 << 13)
		#define LEDRED_OFF()        LATBSET  = (1 << 13)
		
		#define LEDGREEN_ENABLE()   TRISBCLR = (1 << 15)
		#define LEDGREEN_ON()      	LATBCLR  = (1 << 15)
		#define LEDGREEN_OFF()      LATBSET  = (1 << 15)
		
		#define LEDBLUE_ENABLE()    TRISBCLR = (1 << 12)
		#define LEDBLUE_ON()       	LATBCLR  = (1 << 12)
		#define LEDBLUE_OFF()       LATBSET  = (1 << 12)
		
		#else
		#define LEDRED_ENABLE()    
		#define LEDRED_ON()       	
		#define LEDRED_OFF()        
		
		#define LEDGREEN_ENABLE()   
		#define LEDGREEN_ON()      	
		#define LEDGREEN_OFF()      
		
		#define LEDBLUE_ENABLE()    
		#define LEDBLUE_ON()       	
		#define LEDBLUE_OFF()       
		
		#endif
	
	
	
	
    #define RTC_UPDATE_INT_MS 60*60*1000
    
    #if defined CROSSCHASM_C5_BT ||  defined CROSSCHASM_C5_CELLULAR
    // RTCC I2C
        #define RTCC_I2C_MODULE     I2C5

        #define ALARMPIN                PORTBbits.RB13
        #define ALARMPIN_TRIS           TRISBbits.TRISB13
        #define ALARMPIN_PCFG           AD1PCFGbits.PCFG13

        #define TRIS_INPUT              1
        #define TRIS_OUTPUT             0

        #define PCFG_ANALOG             0
        #define PCFG_DIGITAL            1

        #define BUS_RELEASE()   TRISFSET = (1 << 4 | 1 << 5);

        #define SDA_RELEASE()   TRISFSET = (1 << 4);
        #define SCL_RELEASE()   TRISFSET = (1 << 5);

        #define SDA_LOW()       do {                 \
                                PORTDCLR = (1 << 4); \
                                TRISDCLR = (1 << 4); \
                                }while(0)

        #define SCL_LOW()       do {                 \
                                PORTDCLR = (1 << 5); \
                                TRISDCLR = (1 << 5); \
                                }while(0)

        #define SCL_STATE       PORTFbits.RF5
        #define SDA_STATE       PORTFbits.RF4

        #define SDA_MAGIC_PIN   0
    #elif defined CROSSCHASM_C5_BLE
    
        #define RTCC_I2C_MODULE     I2C1    
        #define ALARMPIN            PORTBbits.RD11
        #define ALARMPIN_TRIS       TRISBbits.TRISD11
        //#define ALARMPIN_PCFG       AD1PCFGbits.PCFG11

        #define TRIS_INPUT              1
        #define TRIS_OUTPUT             0

        #define PCFG_ANALOG             0
        #define PCFG_DIGITAL            1

        #define BUS_RELEASE()   TRISDSET = (1 << 9 | 1 << 10);

        #define SDA_RELEASE()   TRISDSET = (1 << 9);
        #define SCL_RELEASE()   TRISDSET = (1 << 10);

        #define SDA_LOW()       do {                 \
                                PORTDCLR = (1 << 9); \
                                TRISDCLR = (1 << 9); \
                                }while(0)

        #define SCL_LOW()       do {                 \
                                PORTDCLR = (1 << 10); \
                                TRISDCLR = (1 << 10); \
                                }while(0)

        #define SCL_STATE       PORTDbits.RD9
        #define SDA_STATE       PORTDbits.RD10

        #define SDA_MAGIC_PIN   0    
        
        
    #endif
    
    #define vCPU_CLOCK_HZ 80000000

#endif

#endif
