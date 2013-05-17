==============
UART Output
==============

You can optionally receive the output data over a UART connection in
addition to USB. The data format is the same as USB - a stream of newline
separated JSON objects.

In the same way that you can send OpenXC writes over USB using the OUT
direction of the USB endpoint, you can send identically formatted
messages in the opposite direction on the serial device - from the host
to the CAN translator. They'll be processed in exactly the same way.
These write messages are accepted via serial even if USB is connected. One
important difference between reads and writes - write JSON messages must be
separated by a NULL character instead of a newline.

For details on your particular platform like the pins and baud rate, see the
:doc:`supported platforms </platforms/platforms>`.
