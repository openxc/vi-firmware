.. _bit-numbering:

============================
Bit Numbering and Byte Order
============================

When dealing with binary data like CAN messages, there are two important details
- byte order and bit numbering.

Byte order, or _`endianness <http://en.wikipedia.org/wiki/Endianness>`, determines
the convention used to interpret a sequence of bytes as a number. Given 4 bytes
of data, e.g. ``0x01 02 03 04``, the endianness determines which byte is the
"zero-th" byte and which is the last. There are only two options: big endian
(a.k.a. Motorola order) and little endian (Intel order). Big endian is most
common in automotive networks.

Bit numbering is an automotive specific term and must less standardized. The bit
numbering can be what we will call normal or inverted. This doesn't change the
actual data representation (like a different *byte* order would) but it changes
how you refer to different bit positions for CAN signals.

The vehicle interface C++ source assumes the number of the highest order bit of
a 64-bit CAN message is 0, and the numbering continuous left to right:

.. code-block:: none

   Hex:         0x83                     46
   Binary:      10000011              01000110
   Bit pos:   0 1 2 3 4 5 6 7   8 9 10 11 12 13 14 15 ...etc.

The most commonly used format in the industry, DBC files from Vector's software,
use an inverted numbering by default when viewed in CANdb++, for example. In
each byte of a CAN message, they start counting bits from the *rightmost bit*,
e.g.:

.. code-block:: none

   Hex:         0x83                     46
   Binary:      10000011              01000110
   Bit pos:   7 6 5 4 3 2 1 0   15 14 13 12 11 10 9 8 ...etc.

You can control the bit numbering with the ``bit_numbering_inverted`` flag on a
message set or message (where the property will cascade down to all signals) or
an individual signal in a VI configuration file. By default the VI assumes
normal bit ordering for each signal **unless** you are using a database-backed
mapping - the DBC files we've seen so far have all stored signal information in
the inverted format.
