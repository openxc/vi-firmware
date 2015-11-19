/*******************************************************************************
 SPI Driver Interface Definition File

  Company:
    Microchip Technology Inc.

  File Name:
    drv_spi.h

  Summary:
    This header file provides APIs for SPI driver. 

  Description:
    This header file provides APIs for SPI driver. 

*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2015 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*******************************************************************************/
// DOM-IGNORE-END

#ifndef _DRV_SPI_H
#define _DRV_SPI_H

#include <stdint.h>


#ifdef __cplusplus  // Provide C++ Compatability

    extern "C" {

#endif

// *****************************************************************************
/* SPI Modes Enumeration

  Summary:
    Specifies the SPI modes which can be used in the initialization structure
    to initialize the SPI for operation. 

  Description:
    Specifies the SPI bus modes enumeration for which an SPI channel
    can operate on. The SPI channel bus mode can be set in the
    SPI channel initialization routine parameter.
*/
 
typedef enum
{
    SPI_BUS_MODE_0 = 0x0050,    //smp = 0, ckp = 0
    SPI_BUS_MODE_1,             //smp = 1, ckp = 0
    SPI_BUS_MODE_2,             //smp = 0, ckp = 1
    SPI_BUS_MODE_3,             //smp = 1, ckp = 1
    
}SPI_BUS_MODES;

// *****************************************************************************
/* SPI Transfer Mode Enumeration

  Summary:
    Defines the Transfer Mode enumeration for SPI.

  Description:
    This defines the Transfer Mode enumeration for SPI.
*/

typedef enum {
    SPI_TRANSFER_MODE_32BIT  = 2,
    SPI_TRANSFER_MODE_16BIT = 1,
    SPI_TRANSFER_MODE_8BIT = 0
}SPI_TRANSFER_MODE;

// *****************************************************************************
/* SPI Initialization structure

  Summary:
    The structure that defines the SPI channel's operation.

  Description:
    Specifies the members which can be adjusted to allow the
    SPI to be initialized for each instance of SPI.
 */

typedef struct
{
    /*Channel for the SPI communication */
    uint16_t            channel;
#if defined (__PIC32MX)
    /*Baud rate for the SPI communication */
    uint16_t            baudRate;
    uint16_t            dummy;
#elif defined (__XC16__)
    /* Primary and Secondary prescalers control the SPI frequency */
    uint16_t            primaryPrescale;
    uint16_t            secondaryPrescale;
#elif defined (__XC8__)
    uint8_t             divider;
#endif
    /* Clock Edge Selection Bits */
    uint8_t            cke;
    /* One of SPI Bus mode as specified SPI_BUS_MODES */
    SPI_BUS_MODES       spibus_mode;
    /* Select between 8 and 16 bit communication */
    SPI_TRANSFER_MODE   mode;
	
} DRV_SPI_INIT_DATA;

/* macros that defines the SPI signal polarities */
    #define SPI_CKE_IDLE_ACT     0        // data change is on active clock to idle clock state
    #define SPI_CKE_ACT_IDLE     1        // data change is on idle clock to active clock state

    #define SPI_CKP_ACT_HIGH     0        // clock active state is high level
    #define SPI_CKP_ACT_LOW      1        // clock active state is low level

    #define SPI_SMP_PHASE_MID    0        // master mode data sampled at middle of data output time 
    #define SPI_SMP_PHASE_END    1        // master mode data sampled at end of data output time

    #define SPI_MST_MODE_ENABLE  1        // SPI master mode enabled
    #define SPI_MST_MODE_DISABLE 0        // SPI master mode disabled, use SPI in slave mode

    #define SPI_MODULE_ENABLE    1        // Enable SPI 
    #define SPI_MODULE_DISABLE   0        // Disable SPI 

// *****************************************************************************
/* Function:
    void DRV_SPI_Initialize(DRV_SPI_INIT_DATA *pData)

  Summary:
    Initializes the SPI instance specified by the channel of the initialization
    structure.

  Description:
    This routine initializes the spi driver instance specified by the channel
    of the initialization structure making it ready for clients to lock and
    use it.

  Precondition:
    None.

  Returns:
    None.

  Parameters:
    pData      - SPI initialization structure.

  Example:
    <code>
    uint16_t           myBuffer[MY_BUFFER_SIZE];
    unsigned int       total;
    uint8_t            myChannel = 2;
    DRV_SPI_INIT_DATA  spiInitData = {2, 3, 7, 0, SPI_BUS_MODE_3, 0};

    DRV_SPI_Initialize(&spiInitData);
    DRV_SPI_Lock(myChannel);

    total = 0;
    do
    {
        total  += DRV_SPI_PutBuffer( myChannel, &myBuffer[total], MY_BUFFER_SIZE - total );

        // Do something else...

    } while( total < MY_BUFFER_SIZE );

    </code>

  Remarks:
    This routine must be called before any other SPI routine is called.
    This routine should only be called once during system initialization.
    Current implementation supports 8-bit transfer mode only.
*/

void DRV_SPI_Initialize(DRV_SPI_INIT_DATA *pData);

// *****************************************************************************
/* Function:
    void DRV_SPI_Deinitialize (uint8_t channel)

  Summary:
    Deinitializes the SPI instance specified by the channel parameter

  Description:
    This routine deinitializes the spi driver instance specified by the channel
    parameter.

  Precondition:
    None.

  Returns:
    None.

  Parameters:
    channel      - SPI instance which needs to be deinitialized.

  Example:
    <code>
    uint8_t            myChannel = 2;

    DRV_SPI_Deinitialize(myChannel);
    </code>

  Remarks:
    None.
*/

void DRV_SPI_Deinitialize (uint8_t channel);


// *****************************************************************************
/* Function:
    void DRV_SPI_Put(uint8_t channel, uint8_t data)

  Summary:
    Writes a byte of data to the SPI to the specified channel

  Description:
    This routine writes a byte of data to the SPI to the specified channel

  Precondition:
    The DRV_SPI_Initialize routine must have been called for the specified
    SPI driver instance.

  Returns:
    None.

  Parameters:
    channel      - SPI instance through which the communication needs to happen

    data         - Data byte to write to the SPI

  Example:
    <code>
    uint16_t        myBuffer[MY_BUFFER_SIZE];
    unsigned int    numBytes;
    uint8_t         myChannel = 2;

    // Pre-initialize myBuffer with MY_BUFFER_SIZE bytes of valid data.

    numBytes = 0;
    while( numBytes < MY_BUFFER_SIZE )
    {
        // DRV_SPI_Put API returns data in any case, upto the user to use it
        DRV_SPI_Put( myChannel, myBuffer[numBytes++] );

        // Do something else...
    }
    </code>

  Remarks:
    This is a blocking routine.
*/

void DRV_SPI_Put(uint8_t channel, uint8_t data);

// *****************************************************************************
/* Function:
    uint8_t DRV_SPI_Get(uint8_t channel)

  Summary:
    Reads a byte of data from SPI from the specified channel

  Description:
    This routine reads a byte of data from SPI from the specified channel

  Precondition:
    The DRV_SPI_Initialize routine must have been called for the specified
    SPI driver instance.

  Returns:
    A data byte received by the driver.

  Parameters:
    channel      - SPI instance through which the communication needs to happen

  Example:
    <code>
    char            myBuffer[MY_BUFFER_SIZE];
    unsigned int    numBytes;
    uint8_t         myChannel = 2;

    numBytes = 0;
    do
    {
        myBuffer[numBytes++] = DRV_SPI_Get(myChannel);
        // Do something else...

    } while( numBytes < MY_BUFFER_SIZE);
    </code>

  Remarks:
    This is blocking routine.
*/

uint8_t DRV_SPI_Get(uint8_t channel);


// *****************************************************************************
/* Function:
    int DRV_SPI_Lock(uint8_t channel)

  Summary:
    Locks the SPI instance specified using the channel parameter

  Description:
    This routine locks the SPI driver instance specified using the channel
    parameter

  Precondition:
    None.

  Returns:
    Returns the status of the driver usage.

  Parameters:
    channel      - SPI instance through which the communication needs to happen

  Example:
    Refer to DRV_SPI_Initialize() for an example

  Remarks:
    None.
*/

int DRV_SPI_Lock(uint8_t channel);


// *****************************************************************************
/* Function:
    void DRV_SPI_Unlock(uint8_t channel)

  Summary:
    Unlocks the SPI instance specified by channel parameter

  Description:
    This routine unlocks the SPI driver instance specified by channel parameter
    making it ready for other clients to lock and use it.

  Precondition:
    None.

  Returns:
    None.

  Parameters:
    channel      - SPI instance through which the communication needs to happen

  Example:
    <code>
        uint8_t myChannel = 2;

        DRV_SPI_Unlock(myChannel);
    </code>

  Remarks:
    None.
*/

void DRV_SPI_Unlock(uint8_t channel);


// *****************************************************************************
/* Function:
    void DRV_SPI_PutBuffer (uint8_t channel, uint8_t * data, uint16_t count)

  Summary:
    Writes a data buffer to SPI

  Description:
    This routine writes a buffered data to SPI.

  Precondition:
    The DRV_SPI_Initialize routine must have been called for the specified
    SPI driver instance.

  Returns:
    None.

  Parameters:
    channel      - SPI instance through which the communication needs to happen

    data         - Pointer to buffer containing the data write to the SPI instance

    count        - Total number of bytes that to write to the SPI instance
                   (must be equal to or less than the size of the buffer)
  Example:
    Refer to DRV_SPI_Initialize() for an example

  Remarks:
    This is a blocking routine.
*/

void DRV_SPI_PutBuffer (uint8_t channel, uint8_t * data, uint16_t count);

// *****************************************************************************
/* Function:
    void DRV_SPI_GetBuffer (uint8_t channel, uint8_t * data, uint16_t count)

  Summary:
    Reads a buffered data from SPI

  Description:
    This routine reads a buffered data from the SPI.

  Precondition:
    The DRV_SPI_Initialize routine must have been called.

  Returns:
    None.

  Parameters:
    channel      - SPI instance through which the communication needs to happen

    data         - Pointer to buffer into which the data read from the SPI instance
                   will be placed.

    count        - Total number of bytes that need to be read from the module
                   instance (must be equal to or less than the size of the
                   buffer)

  Remarks:
    This is a blocking routine.
*/

void DRV_SPI_GetBuffer (uint8_t channel, uint8_t * data, uint16_t count);


// *****************************************************************************
/* Function:
    void SPI_DummyDataSet(
                            uint8_t channel,
                            uint8_t dummyData)

  Summary:
    Sets the dummy data when calling exchange functions

  Description:
    This function sets the dummy data used when performing a an SPI 
    get call. When get is used, the exchange functions will still need
    to send data for proper SPI operation.

  Precondition:
    The DRV_SPI_Initialize routine must have been called.

  Returns:
    None.

  Parameters:
    channel      - SPI instance through which the communication needs to happen

    dummyData    - Dummy data to be used.

  Remarks:
    This is a blocking routine.
*/

void SPI_DummyDataSet(
                        uint8_t channel,
                        uint8_t dummyData);

#ifdef __cplusplus  // Provide C++ Compatibility

    }

#endif

#endif // #ifndef _DRV_SPI_H

/*******************************************************************************
 End of File
*/

