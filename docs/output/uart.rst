==============
UART Output
==============

You can optionally receive the output data over a serial connection in
addition to USB. The data format is the same as USB - a stream of newline
separated JSON objects.

In the same way that you can send OpenXC writes over USB using the OUT
direction of the USB endpoint, you can send identically formatted
messages in the opposite direction on the serial device - from the host
to the CAN translator. They'll be processed in exactly the same way.
These write messages are accepted via serial even if USB is connected.

For details on your particular platform, see the :doc:`supported platforms
</platforms/platforms>`.
