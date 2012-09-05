/*
* Copyright(C) NXP Semiconductors, 2011
* All rights reserved.
*
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* LPC products.  This software is supplied "AS IS" without any warranties of
* any kind, and NXP Semiconductors and its licensor disclaim any and 
* all warranties, express or implied, including all implied warranties of 
* merchantability, fitness for a particular purpose and non-infringement of 
* intellectual property rights.  NXP Semiconductors assumes no responsibility
* or liability for the use of the software, conveys no license or rights under any
* patent, copyright, mask work right, or any other intellectual property rights in 
* or to any products. NXP Semiconductors reserves the right to make changes
* in the software without notification. NXP Semiconductors also makes no 
* representation or warranty that such application will be suitable for the
* specified use without further testing or modification.
* 
* Permission to use, copy, modify, and distribute this software and its 
* documentation is hereby granted, under NXP Semiconductors' and its 
* licensor's relevant copyrights in the software, without fee, provided that it 
* is used in conjunction with NXP Semiconductors microcontrollers.  This 
* copyright, permission, and disclaimer notice must appear in all copies of 
* this code.
*/

#define  __INCLUDE_FROM_USB_DRIVER
#include "../USBMode.h"

#if defined(USB_CAN_BE_HOST)

#include "../PipeStream.h"

uint8_t Pipe_Discard_Stream(uint16_t Length,
                            uint16_t* const BytesProcessed)
{
	uint8_t  ErrorCode;
	uint16_t BytesInTransfer = 0;
	
//	Pipe_SetPipeToken(PIPE_TOKEN_IN);

	if ((ErrorCode = Pipe_WaitUntilReady()))
	  return ErrorCode;

	if (BytesProcessed != NULL)
	  Length -= *BytesProcessed;

	while (Length)
	{
		if (!(Pipe_IsReadWriteAllowed()))
		{
			Pipe_ClearIN();
				
			if (BytesProcessed != NULL)
			{
				*BytesProcessed += BytesInTransfer;
				return PIPE_RWSTREAM_IncompleteTransfer;
			}

			if ((ErrorCode = Pipe_WaitUntilReady()))
			  return ErrorCode;
		}
		else
		{
			Pipe_Discard_8();
			
			Length--;
			BytesInTransfer++;
		}
	}

	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Null_Stream(uint16_t Length,
                         uint16_t* const BytesProcessed)
{
	if (BytesProcessed != NULL)
	  Length -= *BytesProcessed;

	while (Length)
	{
		Pipe_Write_8(0);
		Length--;
	}

	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Write_Stream_LE(const void* const Buffer,
			                 uint16_t Length,
			                 uint16_t* const BytesProcessed)
{
	uint8_t* DataStream = (uint8_t*) Buffer;
	if(BytesProcessed != NULL)
	{
		Length -= *BytesProcessed;
		DataStream += *BytesProcessed;
	}

	while(Length)
	{
		Pipe_Write_8(*DataStream);
		DataStream++;
		Length--;
	}

	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Read_Stream_LE(void* const Buffer,
			                uint16_t Length,
			                uint16_t* const BytesProcessed) /* TODO Blocking due to Pipe_WaitUntilReady */
{
	uint8_t* DataStream = (uint8_t *) Buffer;
	uint8_t ErrorCode;

	if ((ErrorCode = Pipe_WaitUntilReady()))
	  return ErrorCode;

	if(BytesProcessed != NULL)
	{
		Length -= *BytesProcessed;
		DataStream += *BytesProcessed;
	}

	while(Length)
	{
		if (Pipe_IsReadWriteAllowed())
		{
			*DataStream = Pipe_Read_8();
			DataStream++;
			Length--;
		}else
		{
			Pipe_ClearIN();
			HcdDataTransfer(PipeInfo[pipeselected].PipeHandle, PipeInfo[pipeselected].Buffer,
											MIN(Length, PipeInfo[pipeselected].BufferSize), &PipeInfo[pipeselected].ByteTransfered);
			if ((ErrorCode = Pipe_WaitUntilReady()))
				return ErrorCode;
		}
	}

	return PIPE_RWSTREAM_NoError;
}
uint8_t Pipe_Write_Stream_BE(const void* const Buffer,
			                             uint16_t Length,
			                             uint16_t* const BytesProcessed)
{
	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Read_Stream_BE(void* const Buffer,
			                            uint16_t Length,
			                            uint16_t* const BytesProcessed)
{
	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Write_PStream_LE(const void* const Buffer,
			                              uint16_t Length,
			                              uint16_t* const BytesProcessed)
{
	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Write_PStream_BE(const void* const Buffer,
			                              uint16_t Length,
			                              uint16_t* const BytesProcessed)
{
	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Write_EStream_LE(const void* const Buffer,
			                              uint16_t Length,
			                              uint16_t* const BytesProcessed)
{
	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Write_EStream_BE(const void* const Buffer,
			                              uint16_t Length,
			                              uint16_t* const BytesProcessed)
{
	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Read_EStream_LE(void* const Buffer,
			                             uint16_t Length,
			                             uint16_t* const BytesProcessed)
{
	return PIPE_RWSTREAM_NoError;
}

uint8_t Pipe_Read_EStream_BE(void* const Buffer,
			                             uint16_t Length,
			                             uint16_t* const BytesProcessed)
{
	return PIPE_RWSTREAM_NoError;
}

#endif

