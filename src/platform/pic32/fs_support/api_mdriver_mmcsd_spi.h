/***************************************************************************
 *
 *            Copyright (c) 2011-2012 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/

#ifndef _API_MDRIVER_MMCSD_SPI_H_
#define _API_MDRIVER_MMCSD_SPI_H_

//#include <GenericTypeDefs.h>
//#include "api_mdriver.h"

//#include "../version/ver_mdriver_mmcsd_spi.h"
//#if VER_MDRIVER_MMCSD_SPI_MAJOR != 1 || VER_MDRIVER_MMCSD_SPI_MINOR != 5
// #error Incompatible MDRIVER_MMCSD_SPI version number!
//#endif

#ifdef __cplusplus
extern "C" {
#endif

// Microchip MSD driver gateway //

#define MEDIA_SECTOR_SIZE 512u

typedef struct
{
    BYTE    errorCode;
    union
    {
        BYTE    value;
        struct
        {
            BYTE    sectorSize  : 1;
            BYTE    maxLUN      : 1;
        }   bits;
    } validityFlags;

    WORD    sectorSize;
    BYTE    maxLUN;
} MEDIA_INFORMATION;

MEDIA_INFORMATION * MDD_SDSPI_MediaInitialize(void);
DWORD MDD_SDSPI_ReadCapacity(void);
WORD MDD_SDSPI_ReadSectorSize(void);
BYTE MDD_SDSPI_MediaDetect(void);
BYTE MDD_SDSPI_SectorRead(DWORD sector_addr, BYTE* buffer);
BYTE MDD_SDSPI_WriteProtectState(void);
BYTE MDD_SDSPI_SectorWrite(DWORD sector_addr, BYTE* buffer, BYTE allowWriteToZero);

// Microchip MSD driver gateway //

//extern F_DRIVER * mmcsd_initfunc ( unsigned long driver_param );

/* Error codes */
#define MMCSD_ERR_NOTPLUGGED      -1     /* for high level */

#define  MMCSD_NO_ERROR           0x00   /*   0 */
#define  MMCSD_ERR_NOTINITIALIZED 0x65   /* 101 */
#define  MMCSD_ERR_INIT           0x66   /* 102 */
#define  MMCSD_ERR_CMD            0x67   /* 103 */
#define  MMCSD_ERR_STARTBIT       0x68   /* 104 */
#define  MMCSD_ERR_BUSY           0x69   /* 105 */
#define  MMCSD_ERR_CRC            0x70   /* 106 */
#define  MMCSD_ERR_WRITE          0x71   /* 107 */
#define  MMCSD_ERR_WRITEPROTECT   0x72   /* 108 */
#define  MMCSD_ERR_NOTAVAILABLE   0x73   /* 109 */

#ifdef __cplusplus
}
#endif

#endif /* _API_MDRIVER_MMCSD_SPI_H_ */

