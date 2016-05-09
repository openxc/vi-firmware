CrossChasm C5 Cellular
======================

The C5 Cellular device requires an OBD-II to DB9 Serial cable (which is included) in the
shipment. This requires the device to be attached elsewhere in the vehicle since it does
not plug directly into the port.

Users are required to supply their own SIM card. The SIM card is the Mini-SIM (2FF) size
that supports 3G GSM.

The C5 Cellular device also has it's own GPS signal which can be configured to be 
inserted into the OpenXC JSON data.

TeLit Power Source
------------------

It is important to note that the TeLit radio (the 3G modem) is powered from the OBD-II
12V power supply. Therefore, in USB powered emulator mode, the radio will not send data 
or respond to commands unless there is also a benchtop 12V supply to the device.


Sample Web Server
-----------------

The below project is a sample Azure server to receive data from a C5 Cellular device.

https://github.com/openxc/openxc-azure-webserver


Configuration Options
---------------------

For full description of all the compile options for the Cellular connection, see 
:doc:`C5 Cellular Configuration Options </advanced/c5_cell_config>`
