#include "interface/fs.h"
#include <stddef.h>
#include "util/log.h"
#include "config.h"

using openxc::util::log::debug;

void openxc::interface::fs::initializeCommon(FsDevice* device) {
    if(device != NULL) {
        device->descriptor.type = InterfaceType::FS;
    }
}

void openxc::interface::fs::deinitializeCommon(FsDevice* device) {
   
}
