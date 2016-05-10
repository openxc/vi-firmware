===============
Real Time Clock
===============
.. _rtc:

Real time Clock Configuration
------------------------------
For correct time stamping it is necessary that the RTC is configured with the correct time. 
A new rtc configuration command is added to the `OpenXC message format <https://github.com/openxc/openxc-message-format>`_.

Example JSON command

{ "command": "rtc_configuration", "unix_time": "1448551563"}


