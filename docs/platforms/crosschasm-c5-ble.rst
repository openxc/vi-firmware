CrossChasm C5 Bluetooth LE
==========================

Bluetooth Low Energy Specifications
------------------------------------
The Bluetooth Low Energy or BLE protocol supports communication through a GATT server - client
architecture. The device hosts a single service with following read and write characteristic UUIDs -

* Service               6800-d38b-423d-4bdb-ba05-c9276d8453e1

* Write Characteristic  6800-d38b-5262-11e5-885d-feff819cdce2

* Notify Characteristic 6800-d38b-5262-11e5-885d-feff819cdce3

The phone application or the GATT client should enable 
notifications on the notify characteristic in order to recieve 
OpenXC messages from the device. The write characteristic
can be used to send commands and configuration messages to the device.


Connection Details
---------------------
To connect to C5_BLE using a host such as a Smart Phone, the application must perform a scan of available 
bluetooth low devices in the network. The C5_BLE device includes the 128 bit service UUID in its scan response 
payload. The phone application should then attempt to connect to this device once a match is found. 
After establishing connection sucessfully over BLE the host application must enable notifications, failing to do so
will result in automatic drop in connection after a timeout.

Sleep Mode
-----------
In the power saving mode the power to the BLE radio is shut-off


LED Lights
-----------
The BLE_C5 has 2 user controllable LEDs. When CAN activity is detected, the green
LED will be enabled. When USB or bluetooth low energy is connected, the blue LED will be enabled. If CAN is silent the red LED will be enabled. All LEDs will be turned off when sleep mode is entered


.. note::

   UART for Debug logging or commands is not available on the C5_BLE device.
   
