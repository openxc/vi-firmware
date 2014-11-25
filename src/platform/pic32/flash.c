/********************************************************************************/
/*																				*/
/*	flash.c	--	Flash Memory Operation Functions								*/
/*																				*/
/********************************************************************************/
/*	Author: 	Gene Apperson													*/
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
/*  Module Description: 														*/
/*																				*/
/*																				*/
/********************************************************************************/
/*  Revision History:															*/
/*																				*/
/*	08/27/2012 <Gene Apperson> Created											*/
/*																				*/
/********************************************************************************/


/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include <p32xxxx.h>
#include <stdint.h>
#include <sys/kmem.h>
#include "flash.h"

/* ------------------------------------------------------------ */
/*				Local Type Definitions							*/
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*				Global Variables								*/
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*				Local Variables									*/
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*				Forward Declarations							*/
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */
/***	eraseFlashPage
**
**	Parameters:
**		adr		- memory address of page to erase
**
**	Return Value:
**		Returns status code
**
**	Errors:
**		Returns zero if operation succeeds, non-zero if not.
**
**	Description:
**		This function will erase the flash memory page specified
**		by adr.
*/

uint32_t eraseFlashPage(void * adr)
{
	uint32_t		st;

	/* Convert the given address into a physical address
	*/
	NVMADDR = KVA_TO_PA((unsigned int) adr);

	/* Perform the erase operation.
	*/
	st = _doNvmOp(nvmopErasePage);

	return st;

}

/* ------------------------------------------------------------ */
/***	writeFlashWord
**
**	Parameters:
**		adr			- word address
**		val			- word value
**
**	Return Value:
**		Returns status of operation
**
**	Errors:
**		Returns 0 if successful, non-zero if not
**
**	Description:
**		Write the specified word to the flash memory at the specified
**		address.
*/

uint32_t writeFlashWord(void * adr, uint32_t val)
{
	uint32_t	st;

	/* Convert the given address into a physical address
	*/
	NVMADDR = KVA_TO_PA((unsigned int) adr);

	/* Place the data in the NVM data register in preparation
	** for writing.
	*/
	NVMDATA = val;

	/* Perform the write operation.
	*/
	st = _doNvmOp(nvmopWriteWord);

	return st;
}

/* ------------------------------------------------------------ */
/***	clearNvmError
**
**	Parameters:
**		none
**
**	Return Value:
**		Returns error status
**
**	Errors:
**		Returns 0 if successful, non-zero if error
**
**	Description:
**		Clear the error status in the NVM controller.
*/

uint32_t clearNvmError()
{

	return _doNvmOp(nvmopNop);

}

/* ------------------------------------------------------------ */
/***	_doNvmOp
**
**	Parameters:
**		nvmop		- NVM operation to perform
**
**	Return Value:
**		Returns status code
**
**	Errors:
**		Returns 0 if success, non-zero if not
**
**	Description:
**		This function performs an operation on the flash memory.
*/

uint32_t __attribute__((nomips16)) _doNvmOp(uint32_t nvmop)
{
	int			nvmSt;
	int			intSt;
	uint32_t	tm;


	// M00TODO: When DMA operations are supported in the core, need
	// to add code here to suspend DMA during the NVM operation.

	intSt = disableInterrupts();

	/* Store the operation code into the NVMCON register.
	*/
	NVMCON = NVMCON_WREN | nvmop;

	/* We need to wait at least 6uS before performing the operation.
	** We can use the core timer to determine elapsed time based on
	** the CPU operating frequency.
	*/
    {
        tm = _CP0_GET_COUNT();
        while (_CP0_GET_COUNT() - tm < ((F_CPU * 6) / 2000000));
    }

	/* Unlock so that we can perform the operation.
	*/
    NVMKEY 		= 0xAA996655;
    NVMKEY 		= 0x556699AA;
    NVMCONSET 	= NVMCON_WR;

    /* Wait for WR bit to clear indicating that the operation has completed.
	*/
    while (NVMCON & NVMCON_WR)
	{
		;
	}

    /* Clear the write enable bit in NVMCON to lock the flash again.
	*/
    NVMCONCLR = NVMCON_WREN;

	//M00TODO: Resume a suspended DMA operation

	restoreInterrupts(intSt);

	/* Return the success state of the operation.
	*/
    return(isNvmError());

}

/* ------------------------------------------------------------ */
/***	ProcName
**
**	Parameters:
**
**	Return Value:
**
**	Errors:
**
**	Description:
**
*/

/* ------------------------------------------------------------ */

/************************************************************************/

