=================
USB Device Driver
=================

Most users do not need to know the details of the device driver, but for
reference it is documented here.

The CAN translator initializes its USB 2.0 controller as a USB device with three
endpoints. The Android tablet or computer you connect to the translator acts as
the USB host, and must initiate all transfers.

Endpoint 0
===========

This is the standard USB control transfer endpoint. The CAN transalator
has a few control commands:

Version
-------

Version control command: ``0x80``

The host can retrieve the version of the CAN translator using the
``0x80`` control request. The data returned is a string containing the
software version of the firmware and the configured vehicle platform in
the format:

::

    Version: 1.0 (c346)

where ``1.0`` is the software version and ``c346`` is the configured
vehicle.

Reset
-----

Reset control command: ``0x81``

The CAN transceivers can be re-initialized by sending the ``0x81``
control request. This command was introduced to work around a bug that
caused the CAN translator to periodically stop responding. The bug still
exists, but there are now workarounds in the code to automatically
re-initialize the transceivers if they stop receiving messages.

Endpoint 1 IN
=============

Endpoint 1 is configured as a bulk transfer endpoint with the ``IN``
direction (device to host). OpenXC JSON messages read from the vehicle
are sent to the host via ``IN`` transactions. When the host is ready to
receive, it should issue a request to read data from this endpoint. A
larger sized request will allow more messages to be batched together
into one USB request and give high overall throughput (with the downside
of introducing delay depending on the size of the request).

Endpoint 2 OUT
==============

OpenXC JSON messages created by the host to send to the vehicle (i.e. to
write to the CAN bus) are sent via ``OUT`` transactions. The CAN
translator is prepared to accept writes from the host as soon as it
initializes USB, so they can be sent at any time. The messages must be separated
by a NULL character.

There is no special demarcation on these messages to indicate they are writes -
the fact that they are written in the ``OUT`` direction is sufficient. Write
messages must be no more than 4 USB packets in size, i.e. 4 \* 64 = 256 bytes.

In the same way the CAN translator is pre-configured with a list of CAN
signals to read and parse from the CAN bus, it is configured with a
whitelist of messages and signals for which to accept writes from the
host. If a message is sent with an unlisted ID it is silently ignored.
