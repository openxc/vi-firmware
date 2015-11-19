#ifndef __RTC_PLATFORM_H_
#define __RTC_PLATFORM_H_

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

#define vCPU_CLOCK_HZ 80000000
#endif