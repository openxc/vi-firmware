/*
 * Copyright(C) 2011, NXP Semiconductor
 * All rights reserved.
 *
 *         LUFA Library
 * Copyright (C) Dean Camera, 2011.
 *
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that it is used in conjunction with NXP Semiconductors
 * microcontrollers.  This copyright, permission, and disclaimer notice
 * must appear in all copies of this code.
 */

#define  __INCLUDE_FROM_USB_DRIVER
#include "USBMode.h"

#ifdef USB_CAN_BE_HOST

#include "../../../Common/Common.h"
#include "USBTask.h"
#include "LPC/HAL/HAL_LPC.h"
#include "USBMemory.h"

/************************************************************************/
/* LOCAL SYMBOL DECLARATIION                                            */
/************************************************************************/
typedef struct MemBlockInfo_t {
	uint32_t size :15; // memory size of this block
	uint32_t isFree :1; // indicate whether this memory block is free or used
	uint32_t next :16; // offset (from head address) to the next block
} sMemBlockInfo, *PMemBlockInfo;

/************************************************************************/
/* LOCAL DEFINE                                                         */
/************************************************************************/
#define ALIGN_FOUR_BYTES    (4) // FIXME only m3 is 1 byte alignment

/* FIXME the following dynamic allocation is temporarly */

#define  HEADER_SIZE                (sizeof(sMemBlockInfo))
#define  HEADER_POINTER(x)          ((uint8_t *)x - sizeof(sMemBlockInfo))
#define  NEXT_BLOCK(x)            ( ((PMemBlockInfo) ( ((x)->next==0) ? NULL : head + (x)->next )) )
#define  LINK_TO_THIS_BLOCK(x)    ( ((uint32_t) ((x)-head)) )

static uint8_t USB_Mem_Buffer[USBRAM_BUFFER_SIZE] ATTR_ALIGNED(4) __DATA(USBRAM_SECTION);

/************************************************************************
 Function    : lpc_memory_init
 Parameters  : void
 Returns     : void
 Description : This function initializes memory pool manager
 ************************************************************************/
void USB_Memory_Init(uint32_t Memory_Pool_Size)
{
	PMemBlockInfo head = (PMemBlockInfo) USB_Mem_Buffer;

	head->next = 0;
	head->size = (Memory_Pool_Size & 0xfffffffc) - HEADER_SIZE ;// align memory size
	head->isFree = 1;
}

/************************************************************************
 Function    : lpc_malloc
 Parameters  : unsigned int    - memory block size
 Returns     : uint8_t *  - Pointer to memory block or NULL
 Description : This function allocates a memory block for the given size
 from memory pool segment
 ************************************************************************/
uint8_t* USB_Memory_Alloc(uint32_t size)
{
	PMemBlockInfo freeBlock, newBlock, blk_ptr = NULL;
	PMemBlockInfo head = (PMemBlockInfo) USB_Mem_Buffer;

	/* Align the requested size by 4 bytes */
	if ((size % ALIGN_FOUR_BYTES) != 0) {
		size = (((size >> 2) << 2) + ALIGN_FOUR_BYTES);
	}

	for (freeBlock = head; freeBlock != NULL; freeBlock = NEXT_BLOCK(freeBlock)) // 1st-fit technique
	{
		if ((freeBlock->isFree == 1) && (freeBlock->size >= size))
		{
			blk_ptr = freeBlock;
			break;
		}
	}

	if (blk_ptr == NULL) {
		return ((uint8_t *) NULL);
	}

	if (blk_ptr->size <= HEADER_SIZE + size) // where (blk_size=size | blk_size=size+HEAD) then allocate whole block & do not create freeBlock
	{
		newBlock = blk_ptr;
		//newBlock->size    = blk_ptr->size;
		//newBlock->next    = blk_ptr->next;
		newBlock->isFree = 0;
	} else {
		/* Locate empty block at end of found block */
		freeBlock = (PMemBlockInfo) (((uint8_t *) blk_ptr) + size + HEADER_SIZE);
		freeBlock->next = blk_ptr->next;
		freeBlock->size = blk_ptr->size - (HEADER_SIZE + size);
		freeBlock->isFree = 1;

		/* Locate new block at start of found block */
		newBlock = blk_ptr;
		newBlock->size = size;
		newBlock->next = LINK_TO_THIS_BLOCK(freeBlock);
		newBlock->isFree = 0;
	}

	return (((uint8_t *) newBlock) + HEADER_SIZE);
}

/************************************************************************
 Function    : lpc_free
 Parameters  : uint8_t *  - Pointer to memory block
 Returns     : None
 Description : This function frees up the given memory block and does
 packing of fragmented blocks
 ************************************************************************/
void USB_Memory_Free(uint8_t *ptr)
{
	PMemBlockInfo prev;
	PMemBlockInfo head = (PMemBlockInfo) USB_Mem_Buffer;
	PMemBlockInfo blk_ptr;

	if (ptr == NULL)
	{
		return;
	}

	blk_ptr = (PMemBlockInfo) HEADER_POINTER(ptr);

	if (blk_ptr->next != 0) // merge with next free block
	{
		if (NEXT_BLOCK(blk_ptr)->isFree == 1)
		{
			blk_ptr->size = blk_ptr->size + NEXT_BLOCK(blk_ptr)->size + HEADER_SIZE;
			blk_ptr->next = NEXT_BLOCK(blk_ptr)->next;
		}
	}

	for (prev = head; prev != NULL; prev = NEXT_BLOCK(prev)) // merge with previous free block
	{
		if (NEXT_BLOCK(prev) == blk_ptr)
		{
			if (prev->isFree == 1)
			{
				prev->size = prev->size + blk_ptr->size + HEADER_SIZE;
				prev->next = blk_ptr->next;
				blk_ptr = prev;
			}
			break;
		}
	}

	blk_ptr->isFree = 1;

	return;
}

#endif
