=============================
I/O, Data Format and Commands
=============================

The OpenXC message format is specified and versioned separately from any of the
individual OpenXC interfaces or libraries, in the `OpenXC Message Format
<https://github.com/openxc/openxc-message-format>`_ repository.

Commands
=========

The firmware supports all commands defined in the OpenXC Message Format
specification. Both UART and USB interfaces accept commands (serialized as JSON)
sent in the normal data stream back to the VI.

The payload format of the commands is specified in the `OpenXC Message Format`_
and will not be repeated here.

The tools from the `OpenXC Python library
<http://python.openxcplatform.com/en/latest/>`_ can send all of these commands
and are a good way to get started. Examples of the terminal commands to use are
included here.

Simple Vehicle Message Writes
-----------------------------

Simple vehicle message write commands require that the firmware is
pre-configured to understand the named signal, and also allows it to be written.

.. code-block:: sh

    openxc-control write --name seat_position --value 20

CAN Message Writes
-------------------------

The CAN message write requires that the VI is configured to allow raw writes
to the given CAN bus. If the ``bus`` attribute is omitted, it will write the
message to the first CAN bus found that permits writes.

.. code-block:: sh

    openxc-control write --bus 1 --id 1234 --value 0x12345678

.. _vehicle-diagnostic-requests:

Diagnostic Requests
---------------------

The diagnostic request command requires that the VI is configured to allow
raw writes to the given CAN bus (with the ``raw_writable`` flag in the config
file). If the ``bus`` attribute is omitted, it will write the message to the
first CAN bus found that permits writes.

.. code-block:: sh

    openxc-diag --bus 1 --id 1234 --mode 1 --pid 5

.. _version-query:

Version Query
-------------

.. code-block:: sh

    openxc-control version

.. _device-id-query:

Device ID Query
----------------

.. code-block:: sh

    openxc-control id

Set CAN Message Passthrough Status
----------------------------------

.. code-block:: sh

    openxc-control set --passthrough --bus 1

Set CAN Acceptance Filter Bypass
----------------------------------

.. code-block:: sh

    openxc-control set --af-bypass --bus 1

Set Payload Format
--------------------

.. code-block:: sh

    openxc-control set --new-payload-format protobuf

UART (Serial, Bluetooth)
========================

The UART (or serial) connection for a VI is often connected to a Bluetooth
module, e.g. the Roving Networks RN-41 on the Ford Reference VI. This allows
wireless I/O  with the VI.

The VI will send all messages it is configured to received out over the UART
interface using the OpenXC message format. The data may be serialized as either
JSON or protocol buffers, depending on the selected output format. Each message
is followed by a ``\r\n`` delimiter.

The UART interface also accepts all valid OpenXC commands. JSON is the only
support format for commands in this version. Commands must be delimited with a
``\0`` (NULL) character.

For details on your particular platform (i.e. the baud rate and pins for UART on
the board) see the :doc:`supported platforms </platforms/platforms>`.

USB Device
===========

The VI is configured as a USB device, so you can connect it to a computer or
mobile device that supports USB OTG. USB is best if you need to stream a lot of
to or from the VI - the UART connection caps out at around 23KB/s, but USB can
go about 100KB/s.

The VI will publish all messages it is configured to received to USB bulk ``IN``
endpoint 2 using the OpenXC message format. The data may be serialized as either
JSON or protocol buffers, depending on the selected output format. Each message
is followed by a ``\r\n`` delimiter. A larger read request from the host request
will allow more messages to be batched together into one USB request and give
high overall throughput (with the downside of introducing delay depending on the
size of the request).

Bulk ``OUT`` endpoint 5 will accept valid OpenXC commands from the host,
serialized as JSON (the Protocol Buffer format is not supported for commands).
Commands must be delimited with a ``\0`` (NULL) character. Commands must
be no more than 256 bytes (4 USB packets).

Finally, the VI publishes log messages to bulk ``IN`` endpoint 11 when compiled
with the ``DEBUG`` flag. The log messages are delimited with ``\r\n``.

If you are using one of the support libraries (e.g. `openxc-python
<https://github.com/openxc/openxc/python>`_ or `openxc-android
<https://github.com/openxc/openxc-android>`_, you don't need to worry about the
details of the USB device driver, but for creating new libraries the endpoints
are documented here.

Control Transfers
-----------------

Transfer request type: ``0x83``

The VI accepts USB control requests on the standard endpoint 0 where the payload
is a standard OpenXC message format command meessage (e.g. version, device ID,
or diagnostic request, etc).

The responses are injected into the normal output data stream usig the same
format as the :ref:`version query <version-query>`, :ref:`device ID query
<device-id-query>`, etc.
