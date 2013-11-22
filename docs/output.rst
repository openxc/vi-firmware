===========================
Output Interfaces & Format
===========================

The OpenXC message format is specified and versioned separately from any of the
individual OpenXC interfaces or libraries, in the `OpenXC Message Format
<https://github.com/openxc/openxc-message-format>`_ repository.

UART Output
==============

You can optionally receive the output data over a UART connection in
addition to USB. The data format is the same as USB - a stream of newline
separated JSON objects.

In the same way that you can send OpenXC writes over USB using the OUT direction
of the USB endpoint, you can send identically formatted messages in the opposite
direction on the serial device - from the host to the VI. They'll be processed
in exactly the same way. These write messages are accepted via serial even if
USB is connected. One important difference between reads and writes - write JSON
messages must be separated by a NULL character instead of a newline.

For details on your particular platform like the pins and baud rate, see the
:doc:`supported platforms </platforms/platforms>`.

USB Device Driver
=================

Most users do not need to know the details of the device driver, but for
reference it is documented here.

The VI initializes its USB 2.0 controller as a USB device with three
endpoints. The Android tablet or computer you connect to the translator acts as
the USB host, and must initiate all transfers.

Endpoint 0
----------

This is the standard USB control transfer endpoint. The VI has a few control
commands:

Version
```````

Version control command: ``0x80``

The host can retrieve the version of the VI using the ``0x80`` control request.
The data returned is a string containing the software version of the firmware
and the configured vehicle platform in the format:

::

    Version: 1.0 (c346)

where ``1.0`` is the software version and ``c346`` is the configured
vehicle.

Reset
`````

Reset control command: ``0x81``

The CAN transceivers can be re-initialized by sending the ``0x81`` control
request. This command was introduced to work around a bug that caused the VI to
periodically stop responding. The bug still exists, but there are now
workarounds in the code to automatically re-initialize the transceivers if they
stop receiving messages.

Endpoint 1 IN
-------------

Endpoint 1 is configured as a bulk transfer endpoint with the ``IN``
direction (device to host). OpenXC JSON messages read from the vehicle
are sent to the host via ``IN`` transactions. When the host is ready to
receive, it should issue a request to read data from this endpoint. A
larger sized request will allow more messages to be batched together
into one USB request and give high overall throughput (with the downside
of introducing delay depending on the size of the request).

Endpoint 1 OUT
--------------

OpenXC JSON messages created by the host to send to the vehicle (i.e. to
write to the CAN bus) are sent via ``OUT`` transactions. The CAN
translator is prepared to accept writes from the host as soon as it
initializes USB, so they can be sent at any time. The messages must be separated
by a NULL character.

There is no special demarcation on these messages to indicate they are writes -
the fact that they are written in the ``OUT`` direction is sufficient. Write
messages must be no more than 4 USB packets in size, i.e. 4 \* 64 = 256 bytes.

In the same way the VI is pre-configured with a list of CAN signals to read and
parse from the CAN bus, it is configured with a whitelist of messages and
signals for which to accept writes from the host. If a message is sent with an
unlisted ID it is silently ignored.
