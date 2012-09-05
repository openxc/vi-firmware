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


#ifndef __LPC_HCD_H__
#define __LPC_HCD_H__

#include "../../../../../Common/Common.h"
#include "../../StdRequestType.h" // FIXME should be USBTask.h instead
#include "../HAL/HAL_LPC.h"
#include <string.h>
#include <stdio.h>

#define HCD_ENDPOINT_MAXPACKET_XFER_LEN					0xFFEEFFEE

/*=======================================================================*/
/*  HCD C O N F I G U R A T I O N                        */
/*=======================================================================*/
#define YES									1
#define NO									0

#define HCD_MAX_ENDPOINT					8	/* Maximum number of endpoints */

#define HC_RESET_TIMEOUT					10			/* in microseconds */
#define TRANSFER_TIMEOUT_MS					1000
#define PORT_RESET_PERIOD_MS				100

/* Control / Bulk transfer is always enabled     */
#define INTERRUPT_LIST_ENABLE				YES	/* Int transfer enable    */
#define ISO_LIST_ENABLE						YES	/* ISO transfer enable    */

/*=======================================================================*/
/*  HCD T Y P E S                        */
/*=======================================================================*/
typedef enum {
	CONTROL_TRANSFER,
	ISOCHRONOUS_TRANSFER,
	BULK_TRANSFER,
	INTERRUPT_TRANSFER
} HCD_TRANSFER_TYPE;

typedef enum {
	SETUP_TRANSFER,
	IN_TRANSFER,
	OUT_TRANSFER
} HCD_TRANSFER_DIR;

typedef enum {
	FULL_SPEED=0,
	LOW_SPEED,
	HIGH_SPEED
} HCD_USB_SPEED;

typedef enum {
	/* TD Completion Code, Corresponding to OHCI Completion Code */
	HCD_STATUS_OK = 0,

	HCD_STATUS_TRANSFER_CRC,
	HCD_STATUS_TRANSFER_BitStuffing,
	HCD_STATUS_TRANSFER_DataToggleMismatch,
	HCD_STATUS_TRANSFER_Stall,
	HCD_STATUS_TRANSFER_DeviceNotResponding,

	HCD_STATUS_TRANSFER_PIDCheckFailure,
	HCD_STATUS_TRANSFER_UnexpectedPID,
	HCD_STATUS_TRANSFER_DataOverrun,
	HCD_STATUS_TRANSFER_DataUnderrun,
	HCD_STATUS_TRANSFER_CC_Reserved1,

	HCD_STATUS_TRANSFER_CC_Reserved2,
	HCD_STATUS_TRANSFER_BufferOverrun,
	HCD_STATUS_TRANSFER_BufferUnderrun,
	HCD_STATUS_TRANSFER_NotAccessed,
	HCD_STATUS_TRANSFER_Reserved,
	/* End Completion Code OHCI 0-15 */

	/*-- 16-20 Data Structure Status --*/
	HCD_STATUS_STRUCTURE_IS_FREE,
	HCD_STATUS_TO_BE_REMOVED,
	HCD_STATUS_TRANSFER_QUEUED,
	HCD_STATUS_TRANSFER_COMPLETED,
	HCD_STATUS_TRANSFER_ERROR,

	/*---------- 21-25 Memory Code ----------*/
	HCD_STATUS_NOT_ENOUGH_MEMORY,
	HCD_STATUS_NOT_ENOUGH_ENDPOINT,
	HCD_STATUS_NOT_ENOUGH_GTD,
	HCD_STATUS_NOT_ENOUGH_ITD,
	HCD_STATUS_NOT_ENOUGH_QTD,

	/* 26-30 */
	HCD_STATUS_NOT_ENOUGH_HS_ITD,
	HCD_STATUS_NOT_ENOUGH_SITD,
	HCD_STATUS_DATA_OVERFLOW,
	HCD_STATUS_DEVICE_DISCONNECTED,
	HCD_STATUS_TRANSFER_TYPE_NOT_SUPPORTED,

	/* 31-35 */
	HCD_STATUS_PIPEHANDLE_INVALID,
	HCD_STATUS_PARAMETER_INVALID
}HCD_STATUS;

//////////////////////////////////////////////////////////////////////////
HCD_STATUS HcdInitDriver (uint8_t HostID);
HCD_STATUS HcdDeInitDriver(uint8_t HostID);
void HcdIrqHandler(uint8_t HostID);

/************************************************************************/
/* Port API                                                                     */
/************************************************************************/
HCD_STATUS HcdRhPortReset(uint8_t HostID, uint8_t PortNum);
HCD_STATUS HcdRhPortEnable(uint8_t HostID, uint8_t PortNum);
HCD_STATUS HcdRhPortDisable(uint8_t HostID, uint8_t PortNum);
HCD_STATUS HcdGetDeviceSpeed(uint8_t HostID, uint8_t PortNum, HCD_USB_SPEED* DeviceSpeed);
uint32_t HcdGetFrameNumber(uint8_t HostID);

/************************************************************************/
/* Pipe API                                                                     */
/************************************************************************/
HCD_STATUS HcdOpenPipe(uint8_t HostID,
					   uint8_t DeviceAddr,
					   HCD_USB_SPEED DeviceSpeed,
					   uint8_t EndpointNo,
					   HCD_TRANSFER_TYPE TransferType,
					   HCD_TRANSFER_DIR TransferDir,
					   uint16_t MaxPacketSize,
					   uint8_t Interval,
					   uint8_t Mult,
					   uint8_t HSHubDevAddr,
					   uint8_t HSHubPortNum,
					   uint32_t* const PipeHandle);

HCD_STATUS HcdClosePipe(uint32_t PipeHandle);
HCD_STATUS HcdCancelTransfer(uint32_t PipeHandle);
HCD_STATUS HcdClearEndpointHalt(uint32_t PipeHandle);

/************************************************************************/
/* Transfer API                                                                     */
/************************************************************************/
HCD_STATUS HcdControlTransfer(uint32_t PipeHandle, const USB_Request_Header_t* const pDeviceRequest, uint8_t* const buffer);
HCD_STATUS HcdDataTransfer(uint32_t PipeHandle, uint8_t* const buffer, uint32_t const length, uint16_t* const pActualTransferred);
HCD_STATUS HcdGetPipeStatus(uint32_t PipeHandle);

#ifdef NXPUSBLIB_DEBUG
	#define hcd_printf			printf
	void assert_status_ok_message(HCD_STATUS status, char const * mess, char const * func, char const * file, uint32_t const line);
#else
	#define hcd_printf(...)
	#define assert_status_ok_message(...)
#endif


#define ASSERT_STATUS_OK_MESSAGE(sts, message) \
	do{\
	HCD_STATUS status = (sts);\
	assert_status_ok_message( status, message, __func__, __FILE__, __LINE__ );\
	if (HCD_STATUS_OK != status) {\
	return status;\
	}\
	}while(0)

#define ASSERT_STATUS_OK(sts)		ASSERT_STATUS_OK_MESSAGE(sts, NULL)

#if defined(__LPC_OHCI_C__) || defined(__LPC_EHCI_C__)

void  HcdDelayUS (uint32_t  delay); // TODO use unify delay
void  HcdDelayMS (uint32_t  delay);
HCD_STATUS OpenPipe_VerifyParameters( uint8_t HostID, uint8_t DeviceAddr, HCD_USB_SPEED DeviceSpeed, uint8_t EndpointNumber, HCD_TRANSFER_TYPE TransferType, HCD_TRANSFER_DIR TransferDir, uint16_t MaxPacketSize, uint8_t Interval, uint8_t Mult );

static __INLINE uint32_t Align32 (uint32_t Value)
{
	return (Value & 0xFFFFFFE0UL); /* Bit 31 .. 5 */
}

static __INLINE uint32_t Aligned (uint32_t alignment, uint32_t Value)
{
	return Value & (~(alignment-1));
}

static __INLINE uint32_t Align4k (uint32_t Value)
{
	return (Value & 0xFFFFF000); /* Bit 31 .. 5 */
}

static __INLINE uint32_t Offset4k(uint32_t Value)
{
	return (Value & 0xFFF);
}
#endif

#endif
