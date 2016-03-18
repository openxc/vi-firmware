#include "interface/fs.h"
#include <stddef.h>
#include "util/log.h"
#include "config.h"

using openxc::util::log::debug;

void openxc::interface::fs::initializeCommon(FsDevice* device) {
    if(device != NULL) {
        device->descriptor.type = InterfaceType::FS;
        QUEUE_INIT(uint8_t,(QUEUE_TYPE(uint8_t)* ) &device->sendQueue);
    }
}

void openxc::interface::fs::deinitializeCommon(FsDevice* device) {
   
}
