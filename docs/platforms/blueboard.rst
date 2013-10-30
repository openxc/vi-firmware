NGX Blueboard LPC1768-H
================================

To build for the Blueboard, compile with the flag ``PLATFORM=BLUEBOARD``.

UART
----

On the LPC17xx, ``UART1`` is used for OpenXC output at the 230000 baud rate.
Like on the chipKIT, hardware flow control (RTS/CTS) is enabled, so CTS must be
pulled low by the receiving device before data will be sent.

- Pin 2.0 - ``UART1 TX``, connect this to the RX line of the receiver.
- Pin 2.1 - ``UART1 RX``, connect this to the TX line of the receiver.
- Pin 2.2 - ``UART1 CTS``, connect this to the RTS line of the receiver.
- Pin 2.7 - ``UART1 RTS``, connect this to the CTS line of the receiver.

UART data is sent only if pin 0.18 is pulled high. If you are using a Bluetooth
module like the `BlueSMiRF <https://www.sparkfun.com/products/10269>`_ from
SparkFun, you need to hard-wire 5v into pin 0.18 to actually enabling UART.
Other hardware implementations (like the :doc:`Ford prototype <ford>`) may be
able to hook the Bluetooth connection status to this pin instead, to make the
status of UART more dynamic.

Debug Logging
-------------

On the Blueboard LPC1768H, logging will be on ``UART0`` at ``115200`` baud:

- Pin 0.2 - ``UART0 TX``, connect this to the RX line of the receiver
- Pin 0.3 - ``UART0 RX``, connect this to the TX line of the receiver

LED Lights
----------

LEDs are not currently supported on the Blueboard.
