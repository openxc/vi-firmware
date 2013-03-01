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

chipKIT Max32
=============

On the chipKIT, ``UART1A`` is used for OpenXC output at the 460800 baud rate.
Hardware flow control (RTS/CTS) is enabled, so CTS must be pulled low by the
receiving device before data will be sent.

``UART1A`` is also used by the USB-Serial connection, so in order to flash the
PIC32, the Tx/Rx lines must be disconnected. Ideally we could leave that UART
interface for debugging, but there are conflicts with all other exposed UART
interfaces when using flow control.

- Pin 0 - ``U1ARX``, connect this to the TX line of the receiver.
- Pin 1 - ``U1ATX``, connect this to the RX line of the receiver.
- Pin 18 - ``U1ARTS``, connect this to the CTS line of the receiver.
- Pin 19 - ``U1ACTS``, connect this to the RTS line of the receiver.

An additional item to consider when using UART: typically you will want to rig
the chipKIT to be self-powered (either from an external power source or the
vehicle) if you're going to use UART for adding Bluetooth support. There's not
much point in being wireless if you still need power from USB.

In that case, move the USB power jumper from the 5v input on the Network Shield
to A0 (analog input 0). Instead of using 5v to power the board, the firmware can
use it to detect if USB is actually attached or not. The benefit of this is that
if you connect USB, then disconnect it, we can detect that in the firmware and
stop wasting time trying to send data over USB. This will dramatically increase
the throughput over UART.

Blueboard
=========

On the NGX Blueboard LPC1768-H, ``UART1`` is used for OpenXC output at the
230000 baud rate. Like on the chipKIT, hardware flow control (RTS/CTS) is
enabled, so CTS must be pulled low by the receiving device before data will be
sent.

- Pin 2.0 - ``UART1 TX``, connect this to the RX line of the receiver.
- Pin 2.1 - ``UART1 RX``, connect this to the TX line of the receiver.
- Pin 2.2 - ``UART1 CTS``, connect this to the RTS line of the receiver.
- Pin 2.7 - ``UART1 RTS``, connect this to the CTS line of the receiver.
