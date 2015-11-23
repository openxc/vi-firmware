/********************************************************************
 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC(R) Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************/

#include "system.h"
#include <xc.h>
#include <stdbool.h>

/** CONFIGURATION Bits **********************************************/
/*
#pragma config GWRP = OFF
#pragma config GSS = OFF
#pragma config GSSK = OFF
#pragma config FNOSC = PRIPLL
#pragma config IESO = OFF
#pragma config POSCMD = HS
#pragma config OSCIOFNC = OFF
#pragma config IOL1WAY = ON
#pragma config FCKSM = CSDCMD
#pragma config FWDTEN = OFF
#pragma config ICS = PGD2
#pragma config JTAGEN = OFF
*/

void SYSTEM_Initialize (void)
{
    USER_SdSpiConfigurePins_2();
}

void USER_SdSpiConfigurePins_2 (void)
{
   LATGSET  = (1 << 9); //disable SD 
   TRISGCLR = (1 << 9); //Set SD CS as output
#ifdef CROSSCHASM_CELLULAR_C5   
   TRISBSET = (1 << 5);
#endif
}

inline void USER_SdSpiSetCs_2(uint8_t a)
{

    if(a > 0)
     LATGSET  = (1 << 9);
    else
     LATGCLR  = (1 << 9);
}

inline bool USER_SdSpiGetCd_2(void)
{
#ifdef CROSSCHASM_CELLULAR_C5
	return(PORTBbits.RB5);
#endif	

   return true;
}

inline bool USER_SdSpiGetWp_2(void)
{
   return false;
}
