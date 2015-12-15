Test Mode Build
=====================

Description
------------
It is often desired to know if a hardware is functional
before performing other operations on the device. The option
`TEST_MODE_ONLY` enables the user to do just that. Currently
supported on CrossChasm C5 hardware this option creates a firmware
build that will check that hardware and glow an onboard LED depending
on the result.



LED STATES
------------
If all tests were a sucess then the green LED will glow on the device.
Failure will result in blinking of red LED after which it will glow continuously
red.
  .. code-block:: sh

     Blink times = 1,  SD Card could not be mounted.
     Blink times = 2,  RTC could not be mounted.
     Blink times = 3,  Bluetooth low energy radio failed to initialize.