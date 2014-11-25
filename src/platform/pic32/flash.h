/********************************************************************************/
/*																				*/
/*	flash.h	--	Interface for Flash Memory Operations Functions					*/
/*																				*/
/********************************************************************************/
/*	Author:		Gene Apperson													*/
/*	Copyright 2012, Digilent Inc.												*/
/********************************************************************************/
//*	
//*	This library is free software; you can redistribute it and/or
//*	modify it under the terms of the GNU Lesser General Public
//*	License as published by the Free Software Foundation; either
//*	version 2.1 of the License, or (at your option) any later version.
//*	
//*	This library is distributed in the hope that it will be useful,
//*	but WITHOUT ANY WARRANTY; without even the implied warranty of
//*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.//*	See the GNU
//*	Lesser General Public License for more details.
//*	
//*	You should have received a copy of the GNU Lesser General
//*	Public License along with this library; if not, write to the
//*	Free Software Foundation, Inc., 59 Temple Place, Suite 330,
//*	Boston, MA	02111-1307	USA
//*	
/********************************************************************************/
/*  File Description:															*/
/*																				*/
/*																				*/
/********************************************************************************/
/*  Revision History:															*/
/*																				*/
/*	08/27/2012 <Gene Apperson> Created											*/
/*																				*/
/********************************************************************************/


/* ------------------------------------------------------------ */
/*					Miscellaneous Declarations					*/
/* ------------------------------------------------------------ */



/* ------------------------------------------------------------ */
/*					General Type Declarations					*/
/* ------------------------------------------------------------ */

/* NVM operation codes.
*/
#define nvmopNop		0x4000
#define	nvmopWriteWord	0x4001
#define	nvmopErasePage	0x4004

#define isNvmError()    (NVMCON & (_NVMCON_WRERR_MASK | _NVMCON_LVDERR_MASK))

/* ------------------------------------------------------------ */
/*					Object Class Declarations					*/
/* ------------------------------------------------------------ */



/* ------------------------------------------------------------ */
/*					Variable Declarations						*/
/* ------------------------------------------------------------ */



/* ------------------------------------------------------------ */
/*					Procedure Declarations						*/
/* ------------------------------------------------------------ */

uint32_t eraseFlashPage (void * adr);
uint32_t writeFlashWord(void * adr, uint32_t val);
uint32_t clearNvmError();
uint32_t __attribute__((nomips16)) _doNvmOp(uint32_t nvmop);

/* ------------------------------------------------------------ */

/************************************************************************/
