=========================
Firmware Developer Guide
=========================

As much as possible, the firmware is written as vanilla C that can compile and
run on any platform. The advantages of this are that we can write unit tests and
run them on a development computer, which really speeds up the
code-compile-debug cycle.

Each embedded platform that the firmware supports needs a set of board support
implementations in the ``platform`` folder. Right now we support the PIC32 (2
different variants) and LPC1768. There are a number of functions declared in
header files that are indicated as "platform-specific" that must be implemented
for each board you want to support. There isn't a good central place to find
this list right now, but if you start from one of the existing platforms it
should be clear what needs to be implemented. The code only uses
platform-specific C extensions and library includes in these ``platform`` folders.

The unit tests are all in ``src/tests``, and the tests implement their own
platform specific code in ``src/tests/platform`` - many of these include test spy
functionality so the unit tests can verify that an LED turned on, a CAN message
was sent to the controller, etc. You can run the test suite with ``make test``.

The firmware runs on bare metal with a `main event loop
<https://github.com/openxc/vi-firmware/blob/master/src/main.cpp>`_. To
understand the firmware, start walking through the code from there, as that is
the primary driver of the application. At a high level it runs this each time
through the loop:

1. Process any received CAN messages that have been queued up during a CAN rx
   interrupt - call any registered handler functions, translate messages and
   queue output to send back to the client via USB/BT/network.
2. Send any pending diagnostic requests to the bus.
3. Handle any data received from a USB/BT/network connection, including incoming
   commands and CAN write requests.
4. Flush all queued outgoing CAN messages to the bus.
5. Update the LEDs to indicate if the CAN bus is connected, and which output
   interface is active.
6. Finally, flush any queued data to the output interfaces (USB/BT/network).

You'll notice when receiving and sending data, we make use of a buffer - this is
to avoid doing very much work in the interrupt handlers for CAN/USB/UART.
