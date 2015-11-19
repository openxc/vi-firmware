// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright 2015 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license), 
please contact mla_licensing@microchip.com
*******************************************************************************/
//DOM-IGNORE-END


/*************************************************************************/
/*  Note:  This file is included as a template of a header file for      */
/*         a new physical layer.                                         */
/*************************************************************************/


#include "GenericTypeDefs.h"
#include "FSconfig.h"
#include "FSDefs.h"

#define FALSE	0
#define TRUE	!FALSE

/****************************************************************/
/*                    YOUR CODE HERE                            */
/* Add any defines here                                         */
/****************************************************************/

#define INITIALIZATION_VALUE		0x55


/***************************************************************/
/*                      END OF YOUR CODE                       */
/***************************************************************/


BYTE MDD_TEMPLATE_InitIO(void);
BYTE MDD_TEMPLATE_MediaDetect(void);
MEDIA_INFORMATION * MDD_TEMPLATE_MediaInitialize(void);
BYTE MDD_TEMPLATE_SectorRead(DWORD sector_addr, BYTE* buffer);
BYTE MDD_TEMPLATE_SectorWrite(DWORD sector_addr, BYTE* buffer, BYTE allowWriteToZero);
extern BYTE gDataBuffer[];
extern BYTE gFATBuffer[];
extern DISK gDiskData;


/****************************************************************/
/*                    YOUR CODE HERE                            */
/* Add prototypes for any custom functions here                 */
/****************************************************************/


/***************************************************************/
/*                      END OF YOUR CODE                       */
/***************************************************************/


