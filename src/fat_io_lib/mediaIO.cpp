#ifdef DATA_LOGGER
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <sstream>
#include "mediaIO.h"
#include "../util/log.h"
#include "../lights.h"
#include "../MassStorageHost.h"

const char version[] = "_v0.2_";

namespace lights = openxc::lights;
int media_init()
{
    // USB INIT
	while (USB_HostState[FlashDisk_MS_Interface.Config.PortNumber] != HOST_STATE_Configured)
	{
	  debug("HOST STATE %d", USB_HostState[FlashDisk_MS_Interface.Config.PortNumber]);
	  MS_Host_USBTask(&FlashDisk_MS_Interface);
	  USB_USBTask();
	  lights::enable(lights::LIGHT_B, lights::COLORS.white);
	}
	lights::enable(lights::LIGHT_B, lights::COLORS.green);
    return 1;
}

int sequence()
{
	if (USB_HostState[FlashDisk_MS_Interface.Config.PortNumber] != HOST_STATE_Configured){
	  MS_Host_USBTask(&FlashDisk_MS_Interface);
	  USB_USBTask();
	  return 0;
	}

	FL_FILE *config;
	debug("created config");
    // Initialise media
    

    // Initialise File IO Library
    fl_init();
    // Attach media access functions to library
    if (fl_attach_media(media_read, media_write) != FAT_INIT_OK)
    {
        debug("ERROR: Media attach failed\n");
		fl_shutdown();
        return 2; 
    }
	//////////////READ ASPECT
	readConfig = fl_fopen("/config.csv", "r");
	unsigned char readBuff[23];
	if(!fl_fread(readBuff, 23, 1, readConfig)){
		debug("ERROR: Read file failed\n");
		fl_fclose(readConfig);
		fl_shutdown();
	}
	fl_fclose(readConfig);
	const char sequence[5] = {readBuff[18], readBuff[19], readBuff[20], readBuff[21], 0};
	int seqint = atoi(sequence);
	if(seqint<9999){
		seqint++;
		char sequence1[4];
		sprintf (sequence1, "%04d", seqint);
		readBuff[18] = sequence1[0];
		readBuff[19] = sequence1[1];
		readBuff[20] = sequence1[2];
		readBuff[21] = sequence1[3];
		readBuff[22] = 0;	
		FL_FILE *newConfig;
		newConfig = fl_fopen("/config.csv", "w");
		
		if (newConfig)
		{
			// Write some data
			int a = fl_fwrite((const char*)readBuff, 1, strlen((const char*)readBuff), newConfig);
			if ( a != strlen((const char*)readBuff))
			{
				debug("ERROR: Write file failed\n");
				fl_fclose(newConfig);
				fl_shutdown();
				return 2;
			}
		}
		else
		{
			debug("ERROR: Create file failed\n");
			return 2;
		}
		// Close file
		fl_fclose(newConfig);
	}
    // List root directory
    fl_listdirectory("/");

    fl_shutdown();
	return 1;
}


void mediaSetup(){
	//Set up USB
	media_init();
	fl_init();
	if (fl_attach_media(media_read, media_write) != FAT_INIT_OK)
	{
		debug("ERROR: Media attach failed\n");
	}
	// List root directory
	fl_listdirectory("/");
	FL_DIR *dataFolder;
	/////CREATE /data/ folder
	int result = fl_is_dir("/data");
	if(result){
		//folder exists
		debug("data folder already exists");
	}
	else{
		//create folder
		int res = fl_createdirectory("/data");
		if(res){
			//folder created
			debug("data folder created");
		}
		else{
			//folder failed to create
			debug("data folder failed");
		}
	}
	//////////////READ ASPECT
	readConfig = fl_fopen("/config.csv", "r");
	unsigned char readBuff[23];
	if(!fl_fread(readBuff, 23, 1, readConfig)){
		debug("ERROR: Read file failed\n");
		fl_fclose(readConfig);
		fl_shutdown();
	}
	fl_fclose(readConfig);
	//create file name
	char vin[17];
	strncpy(vin, (const char *)readBuff, sizeof(vin));
	const char seq[5] = {readBuff[18], readBuff[19], readBuff[20], readBuff[21], 0};
	const char *data = "/data/";
	const char *json = ".json";
	char fileName[50];
	strncpy(fileName, data, strlen(data));
	strncat(fileName, vin, sizeof(vin));
	strncat(fileName, version, sizeof(version));
	strncat(fileName, seq, 4);
	strncat(fileName, json, strlen(json));
	strncat(fileName, "\0", sizeof("\0"));
	file = fl_fopen((const char*)fileName, "a+");
	if(!file)
	{
		debug("file opened FAILED");
	}else debug("Error file create FAILED");
}

void writeToUSB(char *message){
	//WRITE TO USB
	int a = fl_fwrite(message, 1, strlen(message), file);
	const char* newline = "\r\n";
	int b = fl_fwrite(newline, 1, strlen(newline), file);
	if(a != strlen(message) || b != strlen(newline))
	{
		debug("error writing to file");
	}
}

void mediaShutdown(){
	//End USB
	fl_fclose(file);
	fl_listdirectory("/");
	fl_shutdown();
	lights::enable(lights::LIGHT_B, lights::COLORS.green);
	sequence();
}

void setWriteStatus(bool status){
	writeOut = status;
}

bool getWriteStatus(){
	return writeOut;
}


int media_read(unsigned long sector, unsigned char *buffer, unsigned long sector_count)
{
    unsigned long i;
	SCSI_Capacity_t DiskCapacity;
	if (MS_Host_ReadDeviceCapacity(&FlashDisk_MS_Interface, 0, &DiskCapacity))
	{
		debug(PSTR("Error retrieving device capacity.\r\n"));
		USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
		//Error with USB drive, turn both lights red to indicate
		lights::enable(lights::LIGHT_A, lights::COLORS.red);
		lights::enable(lights::LIGHT_B, lights::COLORS.red);
		return 0;
	}
    for (i=0;i<sector_count;i++)
    {
        if (MS_Host_ReadDeviceBlocks(&FlashDisk_MS_Interface, 0, sector, sector_count, DiskCapacity.BlockSize, buffer))
		{
			debug(PSTR("Error reading device block.\r\n"));
			USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
			return 0;
		}

        sector ++;
        buffer += 512;
    }

    return 1;
}

int media_write(unsigned long sector, unsigned char *buffer, unsigned long sector_count)
{
	//debug("enter media write");
    unsigned long i;
	SCSI_Capacity_t DiskCapacity;
	if (MS_Host_ReadDeviceCapacity(&FlashDisk_MS_Interface, 0, &DiskCapacity))
	{
		debug(PSTR("Error retrieving device capacity.\r\n"));
		USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
		//Error with USB drive, turn both lights red to indicate
		lights::enable(lights::LIGHT_A, lights::COLORS.red);
		lights::enable(lights::LIGHT_B, lights::COLORS.red);
		return 0;
	}
    for (i=0;i<sector_count;i++)
    {
        if (MS_Host_WriteDeviceBlocks(&FlashDisk_MS_Interface, 0, sector, sector_count, DiskCapacity.BlockSize, buffer))
		{
			debug("Error writing BlockBuffer");
			USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
			return 0;
		}
		//debug("Success Writing BlockBuffer");

        sector ++;
        buffer += 512;
    }
	//debug("exit media write");
    return 1;
}

void EVENT_USB_Host_DeviceAttached(const uint8_t corenum)
{
	printf_P(PSTR("Device Attached on port %d\r\n"),corenum);
	//LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}


void EVENT_USB_Host_DeviceUnattached(const uint8_t corenum)
{
	printf_P(PSTR("\r\nDevice Unattached on port %d\r\n"),corenum);
	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}


void EVENT_USB_Host_DeviceEnumerationComplete(const uint8_t corenum)
{
	//LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);

	uint16_t ConfigDescriptorSize;
	uint8_t  ConfigDescriptorData[512];

	if (USB_Host_GetDeviceConfigDescriptor(corenum, 1, &ConfigDescriptorSize, ConfigDescriptorData,
	                                       sizeof(ConfigDescriptorData)) != HOST_GETCONFIG_Successful)
	{
		puts_P(PSTR("Error Retrieving Configuration Descriptor.\r\n"));
		//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		return;
	}

	FlashDisk_MS_Interface.Config.PortNumber = corenum;

	if (MS_Host_ConfigurePipes(&FlashDisk_MS_Interface,
	                           ConfigDescriptorSize, ConfigDescriptorData) != MS_ENUMERROR_NoError)
	{
		puts_P(PSTR("Attached Device Not a Valid Mass Storage Device.\r\n"));
		//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		return;
	}

	if (USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,1) != HOST_SENDCONTROL_Successful)
	{
		puts_P(PSTR("Error Setting Device Configuration.\r\n"));
		//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		return;
	}

	uint8_t MaxLUNIndex;
	if (MS_Host_GetMaxLUN(&FlashDisk_MS_Interface, &MaxLUNIndex))
	{
		puts_P(PSTR("Error retrieving max LUN index.\r\n"));
		//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
		return;
	}

	printf_P(PSTR("Total LUNs: %d - Using first LUN in device.\r\n"), (MaxLUNIndex + 1));

	if (MS_Host_ResetMSInterface(&FlashDisk_MS_Interface))
	{
		puts_P(PSTR("Error resetting Mass Storage interface.\r\n"));
		//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
		return;
	}

	SCSI_Request_Sense_Response_t SenseData;
	if (MS_Host_RequestSense(&FlashDisk_MS_Interface, 0, &SenseData) != 0)
	{
		puts_P(PSTR("Error retrieving device sense.\r\n"));
		//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
		return;
	}

	if (MS_Host_PreventAllowMediumRemoval(&FlashDisk_MS_Interface, 0, true))
	{
		puts_P(PSTR("Error setting Prevent Device Removal bit.\r\n"));
		//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
		return;
	}

	SCSI_Inquiry_Response_t InquiryData;
	if (MS_Host_GetInquiryData(&FlashDisk_MS_Interface, 0, &InquiryData))
	{
		puts_P(PSTR("Error retrieving device Inquiry data.\r\n"));
		//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		USB_Host_SetDeviceConfiguration(FlashDisk_MS_Interface.Config.PortNumber,0);
		return;
	}

	printf_P(PSTR("Vendor \"%.8s\", Product \"%.16s\"\r\n"), InquiryData.VendorID, InquiryData.ProductID);

	puts_P(PSTR("Mass Storage Device Enumerated.\r\n"));
	//LEDs_SetAllLEDs(LEDMASK_USB_READY);
}


void EVENT_USB_Host_HostError(const uint8_t corenum, const uint8_t ErrorCode)
{
	USB_Disable();

	printf_P(PSTR(ESC_FG_RED "Host Mode Error\r\n"
							 " -- Error port %d\r\n"
	                         " -- Error Code %d\r\n" ESC_FG_WHITE), corenum, ErrorCode);

	//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
	for(;;);
}


void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t corenum,
											const uint8_t ErrorCode,
                                            const uint8_t SubErrorCode)
{
	printf_P(PSTR(ESC_FG_RED "Dev Enum Error\r\n"
							 " -- Error port %d\r\n"
	                         " -- Error Code %d\r\n"
	                         " -- Sub Error Code %d\r\n"
	                         " -- In State %d\r\n" ESC_FG_WHITE),
	                         corenum, ErrorCode, SubErrorCode, USB_HostState[corenum]);

	//LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
}
#endif