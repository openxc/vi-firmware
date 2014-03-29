.. _bit-numbering:

=============
Bit Numbering
=============

Because of different software tools and conventions in the industry, there are
multiple ways to refer to bits within a CAN message. This doesn't change the
actual data representation (like a different *byte* order would) but it changes
how you refer to different bit positions for CAN signals.

The vehicle interface C++ source assumes the number of the highest order bit of
a 64-bit CAN message is 0, and the numbering continuous left to right:

.. code-block:: none

   Hex:         0x83                     46
   Binary:      10000011              01000110
   Bit pos:   0 1 2 3 4 5 6 7   8 9 10 11 12 13 14 15 ...etc.

The tool used at Ford to document CAN messages (Vector DBC files) uses an
"inverted" numbering by default. In each byte of a CAN message, they start
counting bits from the *rightmost bit*, e.g.:

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
