/*********************************************************************
  File Information:
    FileName:        usb_function_msd.c
    Dependencies:    See INCLUDES section below
    Processor:       PIC18, PIC24, or PIC32
    Compiler:        C18, C30, or C32
    Company:         Microchip Technology, Inc.

    Software License Agreement

    The software supplied herewith by Microchip Technology Incorporated
    (the “Company”) for its PICmicro® Microcontroller is intended and
    supplied to you, the Company’s customer, for use solely and
    exclusively on Microchip PICmicro Microcontroller products. The
    software is owned by the Company and/or its supplier, and is
    protected under applicable copyright laws. All rights are reserved.
    Any use in violation of the foregoing restrictions may subject the
    user to criminal sanctions under applicable laws, as well as to
    civil liability for the breach of the terms and conditions of this
    license.

    THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
    WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
    TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
    IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
    CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

  Summary:
    This file contains functions, macros, definitions, variables,
    datatypes, etc. that are required for use of the MSD function
    driver. This file should be included in projects that use the MSD
    \function driver.
    
    
    
    This file is located in the "\<Install Directory\>\\Microchip\\USB\\MSD
    Device Driver" directory.

  Description:
    USB MSD Function Driver File
    
    This file contains functions, macros, definitions, variables,
    datatypes, etc. that are required for use of the MSD function
    driver. This file should be included in projects that use the MSD
    \function driver.
    
    This file is located in the "\<Install Directory\>\\Microchip\\USB\\MSD
    Device Driver" directory.
    
    When including this file in a new project, this file can either be
    referenced from the directory in which it was installed or copied
    directly into the user application folder. If the first method is
    chosen to keep the file located in the folder in which it is installed
    then include paths need to be added so that the library and the
    application both know where to reference each others files. If the
    application folder is located in the same folder as the Microchip
    folder (like the current demo folders), then the following include
    paths need to be added to the application's project:
    
    ..\\..\\Microchip\\Include
    .
    
    If a different directory structure is used, modify the paths as
    required. An example using absolute paths instead of relative paths
    would be the following:
    
    C:\\Microchip Solutions\\Microchip\\Include
    
    C:\\Microchip Solutions\\My Demo Application

Change History:
  Rev         Description
  ----------  ----------------------------------------------------------
  2.6 - 2.7a  No change
  2.7b		  Improved error case checking, error case recovery, and 
			  sense key/status reporting to the host.
  			  Implemented adjustable read/write failure retry feature.
			  Fixed minor bug that would have interfered with supporting 
			  more than 7 LUNs simultaneously.

********************************************************************/
 
/** I N C L U D E S **************************************************/
#include "USB/USB.h"
#include "HardwareProfile.h"
//#include "FSConfig.h"

#include "USB/usb_function_msd.h"

#define USB_USE_MSD

#ifdef USB_USE_MSD

#if MAX_LUN == 0
    #define LUN_INDEX 0
#else
    #define LUN_INDEX gblCBW.bCBWLUN
#endif

#if defined(__C30__) || defined(__C32__)
    #if defined(USE_INTERNAL_FLASH)
        #include "MDD File System/Internal Flash.h"
    #endif

    #if defined(USE_SD_INTERFACE_WITH_SPI)
        #include "MDD File System/SD-SPI.h"
    #endif

    extern LUN_FUNCTIONS LUN[MAX_LUN + 1];
    #define LUNMediaInitialize()                LUN[LUN_INDEX].MediaInitialize()
    #define LUNReadCapacity()                   LUN[LUN_INDEX].ReadCapacity()
    #define LUNReadSectorSize()                 LUN[LUN_INDEX].ReadSectorSize()
    #define LUNMediaDetect()                    LUN[LUN_INDEX].MediaDetect()
    #define LUNSectorWrite(bLBA,pDest,Write0)   LUN[LUN_INDEX].SectorWrite(bLBA, pDest, Write0)
    #define LUNWriteProtectState()              LUN[LUN_INDEX].WriteProtectState()
    #define LUNSectorRead(bLBA,pSrc)            LUN[LUN_INDEX].SectorRead(bLBA, pSrc)
#else
    #if defined(USE_INTERNAL_FLASH)
        #include "MDD File System/Internal Flash.h"
    #endif

    #if defined(USE_SD_INTERFACE_WITH_SPI)
        #include "MDD File System/SD-SPI.h"
    #endif

    #define LUNMediaInitialize()                MDD_MediaInitialize()
    #define LUNReadCapacity()                   MDD_ReadCapacity()
    #define LUNReadSectorSize()                 MDD_ReadSectorSize()
    #define LUNMediaDetect()                    MDD_MediaDetect()
    #define LUNSectorWrite(bLBA,pDest,Write0)   MDD_SectorWrite(bLBA, pDest, Write0)
    #define LUNWriteProtectState()              MDD_WriteProtectState()
    #define LUNSectorRead(bLBA,pSrc)            MDD_SectorRead(bLBA, pSrc)
#endif

//Adjustable user options
#define MSD_FAILED_READ_MAX_ATTEMPTS  (BYTE)100u    //Used for error case handling
#define MSD_FAILED_WRITE_MAX_ATTEMPTS (BYTE)100u    //Used for error case handling

/** V A R I A B L E S ************************************************/
#pragma udata
BYTE MSD_State;			// Takes values MSD_WAIT, MSD_DATA_IN or MSD_DATA_OUT
BYTE MSDReadState;
BYTE MSDWriteState;
BYTE MSDRetryAttempt;
USB_MSD_CBW gblCBW;	
BYTE gblCBWLength;
RequestSenseResponse gblSenseData[MAX_LUN + 1];
BYTE *ptrNextData;
USB_HANDLE USBMSDOutHandle;
USB_HANDLE USBMSDInHandle;
WORD MSBBufferIndex;
WORD gblMediaPresent; 
BOOL SoftDetach[MAX_LUN + 1];
static BYTE MSDCommandState = MSD_COMMAND_WAIT;

static WORD_VAL TransferLength;
static DWORD_VAL LBA;

/* 
 * Number of Blocks and Block Length are global because 
 * for every READ_10 and WRITE_10 command need to verify if the last LBA 
 * is less than gblNumBLKS	
 */	
DWORD_VAL gblNumBLKS,gblBLKLen;	
extern const ROM InquiryResponse inq_resp;

/** P R I V A T E  P R O T O T Y P E S ***************************************/
BYTE MSDProcessCommand(void);
BYTE MSDReadHandler(void);
BYTE MSDWriteHandler(void);
void ResetSenseData(void);

/** D E C L A R A T I O N S **************************************************/
#pragma code

/** C L A S S  S P E C I F I C  R E Q ****************************************/

/******************************************************************************
  Function:
    void USBMSDInit(void)
    
  Summary:
    This routine initializes the MSD class packet handles, prepares to
    receive a MSD packet, and initializes the MSD state machine. This
    \function should be called once after the device is enumerated.

  Description:
    This routine initializes the MSD class packet handles, prepares to
    receive a MSD packet, and initializes the MSD state machine. This
    \function should be called once after the device is enumerated.
    
    Typical Usage:
    <code>
    void USBCBInitEP(void)
    {
        USBEnableEndpoint(MSD_DATA_IN_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        USBMSDInit();
    }
    </code>
  Conditions:
    The device should already be enumerated with a configuration that
    supports MSD before calling this function.
    
  Paramters: None

  Remarks:
    None                                                                                                          
  ****************************************************************************/	
void USBMSDInit(void)
{
    USBMSDInHandle = 0;
    USBMSDOutHandle = USBRxOnePacket(MSD_DATA_OUT_EP,(BYTE*)&msd_cbw,MSD_OUT_EP_SIZE);
    MSD_State = MSD_WAIT;
    MSDCommandState = MSD_COMMAND_WAIT;
    MSDReadState = MSD_READ10_WAIT;
    MSDWriteState = MSD_WRITE10_WAIT;
    gblNumBLKS.Val = 0;
    gblBLKLen.Val = 0;

    gblMediaPresent = 0;

    //For each of the possible logical units
    for(gblCBW.bCBWLUN=0;gblCBW.bCBWLUN<(MAX_LUN + 1);gblCBW.bCBWLUN++)
    {
        //clear all of the soft detach variables
        SoftDetach[gblCBW.bCBWLUN] =  FALSE;
	
        //see if the media is attached
        if(LUNMediaDetect())
        {

            //initialize the media
            if(LUNMediaInitialize())
            {
                //if the media was present and successfully initialized
                //  then mark and indicator that the media is ready
                gblMediaPresent |= ((WORD)1<<gblCBW.bCBWLUN);
            }
        }
        ResetSenseData();
    }
}

/******************************************************************************
 	Function:
 		void USBCheckMSDRequest(void)

 	Summary:
 		This routine handles MSD specific request that happen on EP0.  
        This function should be called from the USBCBCheckOtherReq() call back 
        function whenever implementing an MSD device.

 	Description:
 		This routine handles MSD specific request that happen on EP0.  These
        include, but are not limited to, the standard RESET and GET_MAX_LUN 
 		command requests.  This function should be called from the 
        USBCBCheckOtherReq() call back function whenever using an MSD device.	

        Typical Usage:
        <code>
        void USBCBCheckOtherReq(void)
        {
            //Since the stack didn't handle the request I need to check
            //  my class drivers to see if it is for them
            USBCheckMSDRequest();
        }
        </code>

 	PreCondition:
 		None
 		
 	Parameters:
 		None
 	
 	Return Values:
 		None
 		
 	Remarks:
 		None
 
 *****************************************************************************/	
void USBCheckMSDRequest(void)
{
    if(SetupPkt.Recipient != USB_SETUP_RECIPIENT_INTERFACE_BITFIELD) return;
    if(SetupPkt.bIntfID != MSD_INTF_ID) return;

	switch(SetupPkt.bRequest)
    {
	    case MSD_RESET:
	        //Host would typically issue this after a STALL event on an MSD
	        //bulk endpoint.  The MSD reset should re-initialize status
	        //so as to prepare for a new CBW.  Any currently ongoing command
	        //block should be aborted, but the STALL and DTS states need to be
	        //maintained (host will re-initialize these seperately using 
	        //CLEAR_FEATURE, endpoint halt).
            MSD_State = MSD_WAIT;
            MSDCommandState = MSD_COMMAND_WAIT;
            MSDReadState = MSD_READ10_WAIT;
            MSDWriteState = MSD_WRITE10_WAIT;
	    	break;
	    case GET_MAX_LUN:
            //If the host asks for the maximum number of logical units
            //  then send out a packet with that information
	    	CtrlTrfData[0] = MAX_LUN;
            USBEP0SendRAMPtr((BYTE*)&CtrlTrfData[0],1,USB_EP0_INCLUDE_ZERO);
	    	break;
    }	//end switch(SetupPkt.bRequest)
}

/*********************************************************************************
  Function:
        BYTE MSDTasks(void)
    
  Summary:
    This function runs the MSD class state machines and all of its
    sub-systems. This function should be called periodically once the
    device is in the configured state in order to keep the MSD state
    machine going.
  Description:
    This function runs the MSD class state machines and all of its
    sub-systems. This function should be called periodically once the
    device is in the configured state in order to keep the MSD state
    machine going.
    
    Typical Usage:
    <code>
    void main(void)
    {
        USBDeviceInit();
        while(1)
        {
            USBDeviceTasks();
            if((USBGetDeviceState() \< CONFIGURED_STATE) ||
               (USBIsDeviceSuspended() == TRUE))
            {
                //Either the device is not configured or we are suspended
                //  so we don't want to do execute any application code
                continue;   //go back to the top of the while loop
            }
            else
            {
                //Keep the MSD state machine going
                MSDTasks();
    
                //Run application code.
                UserApplication();
            }
        }
    }
    </code>
  Conditions:
    None
  Return Values:
    BYTE -  the current state of the MSD state machine the valid values are
            defined in MSD.h under the MSDTasks state machine declaration section.
            The possible values are the following\:
            * MSD_WAIT
            * MSD_DATA_IN
            * MSD_DATA_OUT
            * MSD_SEND_CSW
  Remarks:
    None                                                                          
  *********************************************************************************/	
BYTE MSDTasks(void)
{
    BYTE i;
    
    switch(MSD_State)
    {
        case MSD_WAIT:
        {
            //If the MSD state machine is waiting for something to happen
            if(!USBHandleBusy(USBMSDOutHandle))
            {
        		//If we received an OUT packet from the host
                //  then copy the data from the buffer to a global
                //  buffer so that we can keep the information but
                //  reuse the buffer
        		gblCBW.dCBWSignature=msd_cbw.dCBWSignature;					
        		gblCBW.dCBWTag=msd_cbw.dCBWTag;
        		gblCBW.dCBWDataTransferLength=msd_cbw.dCBWDataTransferLength;
            	gblCBW.bCBWFlags=msd_cbw.bCBWFlags;
            	gblCBW.bCBWLUN=msd_cbw.bCBWLUN;
        	    gblCBW.bCBWCBLength=msd_cbw.bCBWCBLength;		// 3 MSB are zero

            	for (i=0;i<msd_cbw.bCBWCBLength;i++)
            	{
            		gblCBW.CBWCB[i]=msd_cbw.CBWCB[i];
                }
            		
                gblCBWLength=USBHandleGetLength(USBMSDOutHandle);

        	    //If this CBW is valid?
        		if ((gblCBWLength==MSD_CBW_SIZE)&&(gblCBW.dCBWSignature==0x43425355)) 
            	{
                    //Is this CBW meaningful?	
       				if((gblCBW.bCBWLUN<=0x0f)
               		&&(gblCBW.bCBWCBLength<=0x10)
               		&&(gblCBW.bCBWCBLength>=0x01)
               		&&(gblCBW.bCBWFlags==0x00||gblCBW.bCBWFlags==0x80)) 
            		{
                		//Prepare the CSW to be sent
                    	msd_csw.dCSWTag=gblCBW.dCBWTag;
                    	msd_csw.dCSWSignature=0x53425355;
                    	
                    	//Keep track of retry attempts, in case of temporary failures
                    	//during processing of a command.
                    	MSDRetryAttempt = 0;
                    	
                    	//Check the command.  With the exception of the REQUEST_SENSE
                    	//command, we should reset the sense key info for each new command block.
                    	//Assume the command will get processed successfully (and hence "NO SENSE" 
                    	//response, which is used for success cases), unless handler code
                    	//later on detects some kind of error.  If it does, it should
                    	//update the sense keys to reflect the type of error detected,
                    	//prior to sending the CSW.
                    	if(gblCBW.CBWCB[0] != MSD_REQUEST_SENSE)
                    	{
                      		gblSenseData[LUN_INDEX].SenseKey=S_NO_SENSE;
        			        gblSenseData[LUN_INDEX].ASC=ASC_NO_ADDITIONAL_SENSE_INFORMATION;
        			        gblSenseData[LUN_INDEX].ASCQ=ASCQ_NO_ADDITIONAL_SENSE_INFORMATION;
    			        }
                    	
        				/* If direction is device to host*/
        				if (gblCBW.bCBWFlags==0x80)
        				{
        					MSD_State=MSD_DATA_IN;
        				}
        				else if (gblCBW.bCBWFlags==0x00) 
            			{
        					/* If direction is host to device*/
        					/* prepare to read data in msd_buffer */
            			    MSD_State=MSD_DATA_OUT;
        				}        								
        			}
        		}
            }
            break;
        }
        case MSD_DATA_IN:
            if(MSDProcessCommand() == MSD_COMMAND_WAIT)
            {
                // Done processing the command, send the status
                MSD_State = MSD_SEND_CSW;
            }
            break;
        case MSD_DATA_OUT:
            if(MSDProcessCommand() == MSD_COMMAND_WAIT)
            {
    			/* Finished receiving the data prepare and send the status */
    		  	if ((msd_csw.bCSWStatus==0x00)&&(msd_csw.dCSWDataResidue!=0)) 
    		  	{
    		  		msd_csw.bCSWStatus=0x02;
    		    }
                MSD_State = MSD_SEND_CSW;
            }
            break;
        case MSD_SEND_CSW:
            if(USBHandleBusy(USBMSDInHandle))
            {
                //The TX buffer is not ready to send the status yet.
                break;
            }
            
            USBMSDInHandle = USBTxOnePacket(MSD_DATA_IN_EP,(BYTE*)&msd_csw,MSD_CSW_SIZE);
            
            //Get ready for next command to come in
            if(!USBHandleBusy(USBMSDOutHandle))
            {
                USBMSDOutHandle = USBRxOnePacket(MSD_DATA_OUT_EP,(BYTE*)&msd_cbw,sizeof(msd_cbw));
            }
        
           	MSD_State=MSD_WAIT;
            break;
            
        default:
            //Illegal condition that should not happen, but might occur if the
            //device firmware incorrectly calls MSDTasks() prior to calling
            //USBMSDInit() during the set-configuration portion of enumeration.
            MSD_State=MSD_WAIT;
    }
    
    return MSD_State;
}

/******************************************************************************
 	Function:
 		void MSDProcessCommandMediaPresent(void)
 		
 	Description:
 		This funtion processes a command received through the MSD
 		class driver
 		
 	PreCondition:
 		None
 		
 	Paramters:
 		None
 	
 	Return Values:
 		BYTE - the current state of the MSDProcessCommand state
 		machine.  The valid values are defined in MSD.h under the 
 		MSDProcessCommand state machine declaration section
 		
 	Remarks:
 		None
 
 *****************************************************************************/	
void MSDProcessCommandMediaPresent(void)
{
    BYTE i; 

    switch(MSDCommandState)
    {
        case MSD_COMMAND_WAIT:
            //copy the received command to the command state machine
            MSDCommandState = gblCBW.CBWCB[0];
            break;
    	case MSD_INQUIRY:
    	{
            //copy the inquiry results from the defined ROM buffer 
            //  into the USB buffer so that it can be transmitted
        	memcpypgm2ram(
        	    (void *)&msd_buffer[0],
        	    (ROM void*)&inq_resp,
        	    sizeof(InquiryResponse)
        	    );
        	msd_csw.dCSWDataResidue=sizeof(InquiryResponse);
        	msd_csw.bCSWStatus=0x00;			// success
        	MSDCommandState = MSD_COMMAND_RESPONSE;
            break;
        }
        case MSD_READ_CAPACITY:
        {
            //If the host asked for the capacity of the device
            DWORD_VAL sectorSize;
            DWORD_VAL capacity;

        	msd_csw.bCSWStatus=0x00;			  // success
            
            //get the information from the physical media
            capacity.Val = LUNReadCapacity();
            sectorSize.Val = LUNReadSectorSize();
            
            //copy the data to the buffer
        	msd_buffer[0]=capacity.v[3];
        	msd_buffer[1]=capacity.v[2];
        	msd_buffer[2]=capacity.v[1];
        	msd_buffer[3]=capacity.v[0];
        	
        	msd_buffer[4]=sectorSize.v[3];
        	msd_buffer[5]=sectorSize.v[2];
        	msd_buffer[6]=sectorSize.v[1];
        	msd_buffer[7]=sectorSize.v[0];
        
        	msd_csw.dCSWDataResidue=0x08;		  // size of response
        	MSDCommandState = MSD_COMMAND_RESPONSE;
            break;
        }
		case MSD_READ_10:
        	if(MSDReadHandler() == MSD_READ10_WAIT)
        	{
			    MSDCommandState = MSD_COMMAND_WAIT;
            }
            break;
    	case MSD_WRITE_10:
        	if(MSDWriteHandler() == MSD_WRITE10_WAIT)
        	{
			    MSDCommandState = MSD_COMMAND_WAIT;
            }
		    break;
        case MSD_REQUEST_SENSE:    
          	for(i=0;i<sizeof(RequestSenseResponse);i++)
          	{
          		msd_buffer[i]=gblSenseData[LUN_INDEX]._byte[i];
            }
          	
            msd_csw.dCSWDataResidue=sizeof(RequestSenseResponse);
          	msd_csw.bCSWStatus=0x0;					// success
          	MSDCommandState = MSD_COMMAND_RESPONSE;
            break;
	    case MSD_MODE_SENSE:
        	msd_buffer[0]=0x02;
        	msd_buffer[1]=0x00;
        	msd_buffer[2]=0x00;
        	msd_buffer[3]=0x00;
        	
        	msd_csw.bCSWStatus=0x0;
        	msd_csw.dCSWDataResidue=0x04;
        	MSDCommandState = MSD_COMMAND_RESPONSE;
    	    break;
		case MSD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            if(LUNMediaDetect())
            {
        		msd_csw.bCSWStatus=0x00;
        		msd_csw.dCSWDataResidue=0x00;
        	}
            else
            {
        		gblSenseData[LUN_INDEX].SenseKey=S_NOT_READY;
        		gblSenseData[LUN_INDEX].ASC=ASC_MEDIUM_NOT_PRESENT;
        		gblSenseData[LUN_INDEX].ASCQ=ASCQ_MEDIUM_NOT_PRESENT;
        		msd_csw.bCSWStatus=0x01;
        	}
			MSDCommandState = MSD_COMMAND_WAIT;
            break;
		case MSD_TEST_UNIT_READY:
            if((gblSenseData[LUN_INDEX].SenseKey==S_UNIT_ATTENTION) && (msd_csw.bCSWStatus==1))
            {
                MSDCommandState = MSD_COMMAND_WAIT;
            }
            else
            {
            	ResetSenseData();
            	msd_csw.dCSWDataResidue=0x00;
    			MSDCommandState = MSD_COMMAND_WAIT;
            }
            break;
		case MSD_VERIFY:
            //Fall through to STOP_START
		case MSD_STOP_START:
        	msd_csw.bCSWStatus=0x0;
        	msd_csw.dCSWDataResidue=0x00;
			MSDCommandState = MSD_COMMAND_WAIT;
            break;
        case MSD_COMMAND_RESPONSE:
            if(USBHandleBusy(USBMSDInHandle) == FALSE)
            {
                USBMSDInHandle = USBTxOnePacket(MSD_DATA_IN_EP,(BYTE*)&msd_buffer[0],msd_csw.dCSWDataResidue);
			    MSDCommandState = MSD_COMMAND_WAIT;

        		msd_csw.dCSWDataResidue=0;
            }
            break;
        case MSD_COMMAND_ERROR:
		default:
        	ResetSenseData();
			gblSenseData[LUN_INDEX].SenseKey=S_ILLEGAL_REQUEST;
			gblSenseData[LUN_INDEX].ASC=ASC_INVALID_COMMAND_OPCODE;
			gblSenseData[LUN_INDEX].ASCQ=ASCQ_INVALID_COMMAND_OPCODE;
			msd_csw.bCSWStatus=0x01;
			msd_csw.dCSWDataResidue=0x00;
			MSDCommandState = MSD_COMMAND_RESPONSE;
 		    break;
	} // end switch	
}

/******************************************************************************
 	Function:
 		void MSDProcessCommandMediaAbsent(void)
 		
 	Description:
 		This funtion processes a command received through the MSD
 		class driver
 		
 	PreCondition:
 		None
 		
 	Parameters:
 		None
 	
 	Return Values:
 		BYTE - the current state of the MSDProcessCommand state
 		machine.  The valid values are defined in MSD.h under the 
 		MSDProcessCommand state machine declaration section
 		
 	Remarks:
 		None
 
  *****************************************************************************/	
void MSDProcessCommandMediaAbsent(void)
{
    BYTE i;

    switch(MSDCommandState)
    {
        case MSD_REQUEST_SENSE:
        {
            ResetSenseData();
            gblSenseData[LUN_INDEX].SenseKey=S_NOT_READY;
    		gblSenseData[LUN_INDEX].ASC=ASC_MEDIUM_NOT_PRESENT;
    		gblSenseData[LUN_INDEX].ASCQ=ASCQ_MEDIUM_NOT_PRESENT;

          	for(i=0;i<sizeof(RequestSenseResponse);i++)
          	{
          		msd_buffer[i]=gblSenseData[LUN_INDEX]._byte[i];
            }
          	
            msd_csw.dCSWDataResidue=sizeof(RequestSenseResponse);
          	msd_csw.bCSWStatus=0x0;					// success
          	MSDCommandState = MSD_COMMAND_RESPONSE;
            break;
        } 
        case MSD_PREVENT_ALLOW_MEDIUM_REMOVAL:
        case MSD_TEST_UNIT_READY:
        {
    		msd_csw.bCSWStatus=0x01;
            MSDCommandState = MSD_COMMAND_WAIT;
            break;
        }
        case MSD_INQUIRY:
        {
        	memcpypgm2ram(
        	    (void *)&msd_buffer[0],
        	    (ROM void*)&inq_resp,
        	    sizeof(InquiryResponse)
        	    );
        	msd_csw.dCSWDataResidue=sizeof(InquiryResponse);
        	msd_csw.bCSWStatus=0x00;			// success
        	MSDCommandState = MSD_COMMAND_RESPONSE;
            break;
        }
        case MSD_COMMAND_WAIT:
        {
            MSDCommandState = gblCBW.CBWCB[0];
            break;
        }
        case MSD_COMMAND_RESPONSE:
            if(USBHandleBusy(USBMSDInHandle) == FALSE)
            {
                USBMSDInHandle = USBTxOnePacket(MSD_DATA_IN_EP,(BYTE*)&msd_buffer[0],msd_csw.dCSWDataResidue);
			    MSDCommandState = MSD_COMMAND_WAIT;

        		msd_csw.dCSWDataResidue=0;
            }
            break;
        default:
        {
            //Stall MSD endpoint IN
            USBStallEndpoint(MSD_DATA_IN_EP,1);
    		msd_csw.bCSWStatus=0x01;
            MSDCommandState = MSD_COMMAND_WAIT;
            break;
        }
    }
}

/******************************************************************************
 	Function:
 		BYTE MSDProcessCommand(void)
 		
 	Description:
 		This funtion processes a command received through the MSD
 		class driver
 		
 	PreCondition:
 		None
 		
 	Paramters:
 		None
 		
 	Return Values:
 		BYTE - the current state of the MSDProcessCommand state
 		machine.  The valid values are defined in MSD.h under the
 		MSDProcessCommand state machine declaration section
 		
 	Remarks:
 		None
 
 *****************************************************************************/	
BYTE MSDProcessCommand(void)
{   
  	if(LUNMediaDetect() == FALSE || SoftDetach[gblCBW.bCBWLUN] == TRUE)
    {
        gblMediaPresent &= ~((WORD)1<<gblCBW.bCBWLUN);

        MSDProcessCommandMediaAbsent();
   	}
    else
    {
        if((gblMediaPresent & ((WORD)1<<gblCBW.bCBWLUN)) == 0)
        {
            if(LUNMediaInitialize())
            {
                gblMediaPresent |= ((WORD)1<<gblCBW.bCBWLUN);

        		gblSenseData[LUN_INDEX].SenseKey=S_UNIT_ATTENTION;
        		gblSenseData[LUN_INDEX].ASC=0x28;
        		gblSenseData[LUN_INDEX].ASCQ=0x00;
                msd_csw.bCSWStatus=0x01;

                MSDProcessCommandMediaPresent();
            }
            else
            {
                MSDProcessCommandMediaAbsent();
            }
        }
        else
        {
            MSDProcessCommandMediaPresent();
        }
    }

    return MSDCommandState;
}

/******************************************************************************
 	Function:
 		BYTE MSDReadHandler(void)
 		
 	Description:
 		This funtion processes a read command received through 
 		the MSD class driver
 		
 	PreCondition:
 		None
 		
 	Parameters:
 		None
 		
 	Return Values:
 		BYTE - the current state of the MSDReadHandler state
 		machine.  The valid values are defined in MSD.h under the 
 		MSDReadHandler state machine declaration section
 		
 	Remarks:
 		None
 
  *****************************************************************************/

BYTE MSDReadHandler(void)
{
    switch(MSDReadState)
    {
        case MSD_READ10_WAIT:
        	LBA.v[3]=gblCBW.CBWCB[2];
        	LBA.v[2]=gblCBW.CBWCB[3];
        	LBA.v[1]=gblCBW.CBWCB[4];
        	LBA.v[0]=gblCBW.CBWCB[5];
        	
        	TransferLength.v[1]=gblCBW.CBWCB[7];
        	TransferLength.v[0]=gblCBW.CBWCB[8];

            //Assume success initially, msd_csw.bCSWStatus will get set to 0x01 
            //or 0x02 later if an error is detected during the actual read sequence.        	
        	msd_csw.bCSWStatus=0x0;
        	msd_csw.dCSWDataResidue=0x0;
        	
            MSDReadState = MSD_READ10_BLOCK;
            //Fall through to MSD_READ_BLOCK
        case MSD_READ10_BLOCK:
            if(TransferLength.Val == 0)
            {
                MSDReadState = MSD_READ10_WAIT;
                break;
            }
            
            TransferLength.Val--;					// we have read 1 LBA
            MSDReadState = MSD_READ10_SECTOR;
            //Fall through to MSD_READ10_SECTOR
        case MSD_READ10_SECTOR:
            //if the old data isn't completely sent yet
            if(USBHandleBusy(USBMSDInHandle) != 0)
            {
                break;
            }
            
            //Try to read a sector worth of data from the media, but check for
            //possible errors.
    		if(LUNSectorRead(LBA.Val, (BYTE*)&msd_buffer[0]) != TRUE)
    		{
				if(MSDRetryAttempt < MSD_FAILED_READ_MAX_ATTEMPTS)
				{
				    MSDRetryAttempt++;
                    break;
				}
				else
				{  
    				//Too many consecutive failed reads have occurred.  Need to
    				//give up and abandon the sector read attempt; something must
    				//be wrong and we don't want to get stuck in an infinite loop.
    				//Need to indicate to the host that a device error occurred.
    				//However, we can't send the CSW immediately, since the host
    				//still expects to receive sector read data on the IN endpoint 
    				//first.  Therefore, we still send dummy bytes, before
    				//we send the CSW with the failed status in it.
    				msd_csw.bCSWStatus=0x01;		// Error 0x01 Refer page#18
                                                    // of BOT specifications
                    //Set error status sense keys, so the host can check them later
                    //to determine how to proceed.
                    gblSenseData[LUN_INDEX].SenseKey=S_MEDIUM_ERROR;
			        gblSenseData[LUN_INDEX].ASC=ASC_NO_ADDITIONAL_SENSE_INFORMATION;
			        gblSenseData[LUN_INDEX].ASCQ=ASCQ_NO_ADDITIONAL_SENSE_INFORMATION;
                }
            }//else we successfully read a sector worth of data from our media

            LBA.Val++;
			msd_csw.dCSWDataResidue=BLOCKLEN_512;//in order to send the
                                                 //512 bytes of data read
                                                 
            ptrNextData=(BYTE *)&msd_buffer[0];
            
            MSDReadState = MSD_READ10_TX_SECTOR;
    
            //Fall through to MSD_READ10_TX_SECTOR
        case MSD_READ10_TX_SECTOR:
            if(msd_csw.dCSWDataResidue == 0)
            {
                MSDReadState = MSD_READ10_BLOCK;
                break;
            }
            
            MSDReadState = MSD_READ10_TX_PACKET;
            //Fall through to MSD_READ10_TX_PACKET
            
        case MSD_READ10_TX_PACKET:
    		/* Write next chunk of data to EP Buffer and send */
            
            //Make sure the endpoint is available before using it.
            if(USBHandleBusy(USBMSDInHandle))
            {
                break;
            }
            //Prepare the USB module to send an IN transaction worth of data to the host.
            USBMSDInHandle = USBTxOnePacket(MSD_DATA_IN_EP,ptrNextData,MSD_IN_EP_SIZE);
            
 			MSDReadState = MSD_READ10_TX_SECTOR;

    		gblCBW.dCBWDataTransferLength-=	MSD_IN_EP_SIZE;
    		msd_csw.dCSWDataResidue-=MSD_IN_EP_SIZE;
    		ptrNextData+=MSD_IN_EP_SIZE;
            break;
        
        default:
            //Illegal condition, should never occur.  In the event that it ever
            //did occur anyway, try to notify the host of the error.
            msd_csw.bCSWStatus=0x02;  //indicate "Phase Error"
            //Advance state machine
            MSDReadState = MSD_READ10_WAIT;
    }
    
    return MSDReadState;
}


/******************************************************************************
 	Function:
 		BYTE MSDWriteHandler(void)
 		
 	Description:
 		This funtion processes a write command received through 
 		the MSD class driver
 		
 	PreCondition:
 		None
 		
 	Parameters:
 		None
 		
 	Return Values:
 		BYTE - the current state of the MSDWriteHandler state
 		machine.  The valid values are defined in MSD.h under the 
 		MSDWriteHandler state machine declaration section
 		
 	Remarks:
 		None
 
 *****************************************************************************/
BYTE MSDWriteHandler(void)
{
    switch(MSDWriteState)
    {
        case MSD_WRITE10_WAIT:
         	/* Read the LBA, TransferLength fields from Command Block
               NOTE: CB is Big-Endian */
        
        	LBA.v[3]=gblCBW.CBWCB[2];
        	LBA.v[2]=gblCBW.CBWCB[3];
        	LBA.v[1]=gblCBW.CBWCB[4];
        	LBA.v[0]=gblCBW.CBWCB[5];
        	TransferLength.v[1]=gblCBW.CBWCB[7];
        	TransferLength.v[0]=gblCBW.CBWCB[8];
        
        	//Initially assume success, unless handler code later encounters an
        	//error condition and sets the status to 0x01 or 0x02.
        	msd_csw.bCSWStatus=0x0;	
        	
        	MSD_State = MSD_WRITE10_BLOCK;
        	//Fall through to MSD_WRITE10_BLOCK
        case MSD_WRITE10_BLOCK:
            if(TransferLength.Val == 0)
            {
                MSDWriteState = MSD_WRITE10_WAIT;
                break;
            }
            
            MSDWriteState = MSD_WRITE10_RX_SECTOR;
            ptrNextData=(BYTE *)&msd_buffer[0];
              
        	msd_csw.dCSWDataResidue=BLOCKLEN_512;
        	
            //Fall through to MSD_WRITE10_RX_SECTOR
        case MSD_WRITE10_RX_SECTOR:
        {
      		/* Read 512B into msd_buffer*/
      		if(msd_csw.dCSWDataResidue>0) 
      		{
                if(USBHandleBusy(USBMSDOutHandle) == TRUE)
                {
                    break;
                }

                USBMSDOutHandle = USBRxOnePacket(MSD_DATA_OUT_EP,ptrNextData,MSD_OUT_EP_SIZE);
                MSDWriteState = MSD_WRITE10_RX_PACKET;
                //Fall through to MSD_WRITE10_RX_PACKET
      	    }
      	    else
      	    {
          		//We finished receiving a sector worth of data from the host.
          		//Check if the media is write protected before deciding what
          		//to do with the data.
          		if(LUNWriteProtectState()) 
                {
                    //The device appears to be write protected.
              	    //Let host know error occurred.  The bCSWStatus flag is also used by
              	    //the write handler, to know not to even attempt the write sequence.
              	    msd_csw.bCSWStatus=0x01;    
              	    
                    //Set sense keys so the host knows what caused the error.
              	    gblSenseData[LUN_INDEX].SenseKey=S_NOT_READY;
              	    gblSenseData[LUN_INDEX].ASC=ASC_WRITE_PROTECTED;
              	    gblSenseData[LUN_INDEX].ASCQ=ASCQ_WRITE_PROTECTED;
              	}
   			    MSDWriteState = MSD_WRITE10_SECTOR;     
      			break;
          	}
        }
        //Fall through to MSD_WRITE10_RX_PACKET
        case MSD_WRITE10_RX_PACKET:
            if(USBHandleBusy(USBMSDOutHandle) == TRUE)
            {
                break;
            }
            
        	gblCBW.dCBWDataTransferLength-=USBHandleGetLength(USBMSDOutHandle);		// 64B read
        	msd_csw.dCSWDataResidue-=USBHandleGetLength(USBMSDOutHandle);
            ptrNextData += MSD_OUT_EP_SIZE;
            
            MSDWriteState = MSD_WRITE10_RX_SECTOR;
            break;
        case MSD_WRITE10_SECTOR:
        {
            //Make sure that no error has been detected, before performing the write
            //operation.  If there was an error, skip the write operation, but allow
            //the TransferLength to continue decrementing, so that we can eventually
            //receive all OUT bytes that the host is planning on sending us.  Only
            //after that is complete will the host send the IN token for the CSW packet,
            //which will contain the bCSWStatus letting it know an error occurred.
      		if(msd_csw.bCSWStatus == 0x00)
      		{
          		if(LUNSectorWrite(LBA.Val, (BYTE*)&msd_buffer[0], (LBA.Val==0)?TRUE:FALSE) != TRUE)
          		{
              		//The write operation failed for some reason.  Keep track of retry
              		//attempts and abort if repeated write attempts also fail.
    				if(MSDRetryAttempt < MSD_FAILED_WRITE_MAX_ATTEMPTS)
    				{
    				    MSDRetryAttempt++;
                        break;
    				}
    				else
    				{  
        				//Too many consecutive failed write attempts have occurred. 
        				//Need to give up and abandon the write attempt.
        				msd_csw.bCSWStatus=0x01;		// Error 0x01 Refer page#18
                                                        // of BOT specifications
                        //Set error status sense keys, so the host can check them later
                        //to determine how to proceed.
                        gblSenseData[LUN_INDEX].SenseKey=S_MEDIUM_ERROR;
    			        gblSenseData[LUN_INDEX].ASC=ASC_NO_ADDITIONAL_SENSE_INFORMATION;
    			        gblSenseData[LUN_INDEX].ASCQ=ASCQ_NO_ADDITIONAL_SENSE_INFORMATION;
                    }              		
          		}
      		}
      
            //One LBA is written (unless an error occurred).  Advance state
            //variables so we can eventually finish handling the CBW request.
      		LBA.Val++;				
      		TransferLength.Val--;      
            MSDWriteState = MSD_WRITE10_BLOCK;
            break;
        } 
        
        default:
            //Illegal condition which should not occur.  If for some reason it
            //does, try to let the host know know an error has occurred.
            msd_csw.bCSWStatus=0x02;    //Phase Error
            MSDWriteState = MSD_WRITE10_WAIT;            
    }
    
    return MSDWriteState;
}

/******************************************************************************
 	Function:
 		void ResetSenseData(void)
 		
 	Description:
 		This routine resets the Sense Data, initializing the
 		structure RequestSenseResponse gblSenseData.
 		
 	PreCondition:
 		None 
 		
 	Parameters:
 		None
 		
 	Return Values:
 		None
 		
 	Remarks:
 		None
 			
  *****************************************************************************/
void ResetSenseData(void) 
{
	gblSenseData[LUN_INDEX].ResponseCode=S_CURRENT;
	gblSenseData[LUN_INDEX].VALID=0;			// no data in the information field
	gblSenseData[LUN_INDEX].Obsolete=0x0;
	gblSenseData[LUN_INDEX].SenseKey=S_NO_SENSE;
	//gblSenseData.Resv;
	gblSenseData[LUN_INDEX].ILI=0;
	gblSenseData[LUN_INDEX].EOM=0;
	gblSenseData[LUN_INDEX].FILEMARK=0;
	gblSenseData[LUN_INDEX].InformationB0=0x00;
	gblSenseData[LUN_INDEX].InformationB1=0x00;
	gblSenseData[LUN_INDEX].InformationB2=0x00;
	gblSenseData[LUN_INDEX].InformationB3=0x00;
	gblSenseData[LUN_INDEX].AddSenseLen=0x0a;	// n-7 (n=17 (0..17))
	gblSenseData[LUN_INDEX].CmdSpecificInfo.Val=0x0;
	gblSenseData[LUN_INDEX].ASC=0x0;
	gblSenseData[LUN_INDEX].ASCQ=0x0;
	gblSenseData[LUN_INDEX].FRUC=0x0;
	gblSenseData[LUN_INDEX].SenseKeySpecific[0]=0x0;
	gblSenseData[LUN_INDEX].SenseKeySpecific[1]=0x0;
	gblSenseData[LUN_INDEX].SenseKeySpecific[2]=0x0;
}

#endif
