====================
Binary Output Format
====================

For applications that need to maximize the amount of data transferred from the
vehicle, the firmware includes an *experimental* binary output format. Instead
of JSON, it uses `Google Protocol Buffers
<https://developers.google.com/protocol-buffers/>`_ to more efficiently pack the
data. Translated-style OpenXC messages are on average 30% smaller when using
protobufs instead of JSON, and raw messages are around 60% smaller. This space
savings comes at the cost of decreased flexibility and increased complexity in
receiving and parsing the data.

The firmware does not currently support *receiving* binary-encoded messages -
CAN write requests must still be sent in JSON.

This output format is supported by the official `OpenXC Android library
<https://github.com/openxc/openxc-android>`_ and `OpenXC Python library
<http://python.openxcplatform.com>`_, both of which will auto-detect the output
format being used by an attached VI. The protobuf objects are defined in an
experimental branch in the `openxc-message-format
<https://github.com/openxc/openxc-message-format/tree/binary-encoding>`_
repository if you wish to add support to another language or environment.

The output stream uses the `common delimiting technique
<https://developers.google.com/protocol-buffers/docs/techniques#streaming>`_ of
writing the length of each protobuf message before the message itself in the
stream.

Compiling with Binary Output
============================

To use the binary output format, compile with the
``DEFAULT_OUTPUT_FORMAT=PROTOBUF`` environment variable set
(see :doc:`all compile-time flags </compile/makefile-opts>`).

Motivation
===========
The default output format encodes data from the vehicle as JSON, using the
OpenXC message format. There are JSON formats for both translated and raw
messages. The format is human-readable and very simple to parse, but it's not
very compact.

For one, it representes everything in human-readable text, so for example, the
number ``999`` (which could be stored in only 10 bits) takes 3 bytes - more than
twice the space.

This isn't an issue for most users, as the output interfaces have plenty of
bandwidth (USB is around 125-160 KB/s and the popular RN-42 Bluetooth tops out
at 23KB/s), and the actual amount of data sent from the pre-compiled binary
firmware (from Ford, for example) is only about 3-6KB/s using JSON.

However, some applications need to pull a lot more data out of the car, perhaps
by reading :doc:`raw CAN messages </advanced/lowlevel>` or reading many signals
at high frequencies. These can quickly overwhelm the output pipe using JSON.
