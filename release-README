OpenXC Vehicle Interface Firmware
=================================

This archive contains pre-compiled builds of the open source OpenXC vehicle
interface firmware in a few common configurations for each supported platform.

For instructions on how to update your hardware device with one of these files,
check out the OpenXC website:

http://openxcplatform.com/vehicle-interface/hardware.html

 ---------------
| Default Build |
 ---------------

The files with `default` in the name are compiled for each VI platform with
these default Makefile options:

        'DEBUG': False,
        'MSD_ENABLE' : False,
        'DEFAULT_FILE_GENERATE_SECS' : 180,
        'BOOTLOADER': True,
        'TEST_MODE_ONLY': False,
        'TRANSMITTER': False,
        'DEFAULT_LOGGING_OUTPUT': "OFF",
        'DEFAULT_METRICS_STATUS': False,
        'DEFAULT_CAN_ACK_STATUS': False,
        'DEFAULT_ALLOW_RAW_WRITE_NETWORK': False,
        'DEFAULT_ALLOW_RAW_WRITE_UART': False,
        'DEFAULT_ALLOW_RAW_WRITE_USB': True,
        'DEFAULT_OUTPUT_FORMAT': "JSON",
        'DEFAULT_RECURRING_OBD2_REQUESTS_STATUS': False,
        'DEFAULT_POWER_MANAGEMENT': "SILENT_CAN",
        'DEFAULT_USB_PRODUCT_ID': 1,
        'DEFAULT_EMULATED_DATA_STATUS': False,
        'DEFAULT_OBD2_BUS': 1,
        'NETWORK': False,

 ----------------
| Emulator Build |
 ----------------

The files with `emulator` in the name are compiled for each VI platform with
the following changes from the default Makefile options:

        'DEFAULT_POWER_MANAGEMENT': "ALWAYS_ON",
        'DEFAULT_EMULATED_DATA_STATUS': True,
  

 -----------------------
| Translated OBD2 Build |
 -----------------------

The files with `translated_obd2` in the name are compiled for each VI platform
with the following changes from the default Makefile options:

        'DEFAULT_RECURRING_OBD2_REQUESTS_STATUS': True,
        'DEFAULT_POWER_MANAGEMENT': "OBD2_IGNITION_CHECK",

This firmware will query the vehicle to see which of a subset of PIDs are
supported (the list:
https://github.com/openxc/vi-firmware/blob/master/src/obd2.cpp#L41) and set up
recurring requests for those that are, giving each a user-friendly name so they
are output as simple vehicle messages, e.g.:

    {"name": "engine_speed", "value": 540}

 --------------------
| Default OBD2 Build |
 --------------------

The files with `obd2` in the name are compiled for each VI platform
with the following changes from the default Makefile options:

        'DEFAULT_POWER_MANAGEMENT': "OBD2_IGNITION_CHECK",

This firmware is ready to received OBD2 requests over USB or Bluetooth (see the
message format: https://github.com/openxc/openxc-message-format) but does *not*
have any pre-defined recurring requests. You probably want this build if you are
sending your own, custom diagnostic requests.

 ------------
| MSD Builds |
 ------------

For all of the above builds, another similar build is done that enables the SD card
(Mass Storage Device) for the C5_BT and C5_CELLULAR platforms. All of the files
have "msd" after the platform in the filename. These are all compiled with the 
additional Makefile options

        'MSD_ENABLE' : True,
        'DEFAULT_FILE_GENERATE_SECS' : 180,


 ---------
| License |
 ---------

These binaries do not contain any closed source components. They are compiled
soley from the open source OpenXC vehicle interface firmware
<http://vi-firmware.openxcplatform.com/> and contains code covered by a few
different open source licenses
<http://vi-firmware.openxcplatform.com/en/latest/license-disclosure.html>.
