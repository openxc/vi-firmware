====================
Mass Storage Device 
====================
.. _msd-storage:

Operation
-----------
MSD mode is supported on ``CROSSCHASM_C5_BT`` and ``CROSSCHASM_C5_CELLULAR`` devices. 
With this mode enabled the devices store messages(except debug log messages)on to 
the SD card storage that is formatted with FAT file system. The maximum storage 
capacity supported is upto 2GB. To view the contents of the SD card the user can unplug
the SD card and connect to a card reader or simply connect the device to a USB port.
In the latter case the device will be mounted as a mass storage device. The user can
thereby view and edit the contents of the SD card. To enable MSD mode set 
``MSD_ENABLE`` as ``1`` in the build configuration options.

USB connected modes
--------------------
With the MSD mode enabled the devices still continue to support the standard USB logging and 
command interface support. However at a time either MSD or standard USB is supported, to decide
which(correct mode), the firmware checks how the device was powered. To enable MSD mode the device
must be unplugged from the vehicle and connected to the USB. In the other scenario where the
device was connected to the vehicle USB will function as standard mode.

.. NOTE::
  To view emulated signals with ``DEFAULT_EMULATED_DATA_STATUS`` set to ``1`` the device must be connected
  to a vehicle or powered by a 12V supply on the OBD connectors
  
File generation and naming convention
--------------------------------------
To maintain unique and sequential files a 8.3 filenaming convention is used in which the first
eight ascii characters represent the unix time in hex of the time the file was created.


File splitting technique
-------------------------
It is desired to have multiple log entries based on time rather than one large file. The build configuration 
option ``DEFAULT_FILE_GENERATE_SECS`` creates a new file each time this interval is elapsed.
A larger interval will yeild to lesser files however the size of the files will depend upon the build configuration
and the speed at which data is generated. By default the option ``DEFAULT_FILE_GENERATE_SECS`` is set to ``180``


Real time Clock Configuration
------------------------------
For correct time stamping it is necessary that the RTC is configured with the correct time. 
A new rtc configuration command is added to the _` OpenXC message format <https://github.com/openxc/openxc-message-format>`.

Example JSON command

{"command":"rtc_configuration", "unix_time":"1448551563"}


SD card status message
------------------------------
It may happen that the SD card which connected has become full or is unformatted. In such a scenario
a message is sent to the user over the USB, Bluetooth and Cellular. Based on this the user can perform
a format by connecting the device over USB or insert a new empty card.

UART Debug Support
-------------------
With ``MSD_ENABLE`` as ``1`` the UART3 on the PIC32 is configured as a I2C peripheral, hence debug logging is 
not supported at this time. 
