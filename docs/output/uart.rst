## Serial Output

You can optionally receive the output data over a serial connection in
addition to USB. The same JSON messages are also sent over UART on
pins 18 and 19 of the chipKIT Max32 at a baud rate of 115200. On LPC17XX boards
like the Blueboard, the baud rate is 921600 and requires that the CTS1 and RTS1
pins are also connected to enabled hardware flow control.

In the same way that you can send OpenXC writes over USB using the OUT direction
of the USB endpoint, you can send identically formatted messages in the opposite
direction on the serial device - from the host to the CAN translator. They'll be
processed in exactly the same way. These write messages are accepted via serial
even if USB is connected.

[input-specs]: https://github.com/openxc/cantranslator/blob/master/README_mappings.mkd
