#ifndef __INTERFACE_FS_H__
#define __INTERFACE_FS_H__

#include <stdlib.h>
#include "interface/interface.h"
#include "util/bytebuffer.h" //to do remove this and add custom type to have 512 size
#include "fs_platforms.h"



typedef enum { //todo should we add this in a new fs.h in platform folder?
    NONE_CONNECTED = 0,
    VI_CONNECTED   = 1,
    USB_CONNECTED  = 2,
} FS_STATE;


namespace openxc {
namespace interface {
namespace fs {


typedef struct {
	InterfaceDescriptor descriptor;
	//since our write speeds are much higher to the SD card we are excluding the queue here
	bool configured;
} FsDevice;

void setmode(FS_STATE mode);

bool setRTC(uint32_t *unixtime);

FS_STATE getmode(void);

/* Public: Perform platform-agnostic Network initialization.
 */
void initializeCommon(FsDevice* device);

/* Initializes the network interface with MAC and IP addresses, starts
 * listening for connections.
 */
bool initialize(FsDevice* device);

bool initialize(FsDevice* device);

/* Data is written to disk if write buffer is full
 *
 */
void write(FsDevice* device, uint8_t *data, uint32_t len);
 

void manager(FsDevice* device);

//Will return status of SD Card/File system
bool connected(FsDevice* device);

//Writes any pending data Unmount SD card release buffers
void deinitialize(FsDevice* device);

//Not sure what to do here 
void deinitializeCommon(FsDevice* device);

} // namespace fs
} // namespace interface
} // namespace openxc

#endif // __INTERFACE_FS_H__