#include "version_command.h"
#include "config.h"
#include "diagnostics.h"
#include "interface/usb.h"
#include "util/log.h"
#include "config.h"
#include "pb_decode.h"
#include <payload/payload.h>
#include "signals.h"
#include <can/canutil.h>
#include <bitfield/bitfield.h>
#include <limits.h>
#include "sd_mount_status_command.h"
#include "interface/fs.h"

using openxc::util::log::debug;


bool openxc::commands::handleSDMountStatusCommand() {

  bool status = false;
#ifdef FS_SUPPORT
  status = openxc::interface::fs::getSDStatus(); //returns true if SD card was initialized correctly
#endif    

  sendCommandResponse(openxc_ControlCommand_Type_SD_MOUNT_STATUS, status,
            NULL, 0);
            
  return status;
    
}

