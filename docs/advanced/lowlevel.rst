======================
Low-level CAN Features
======================

The OpenXC message format specification defines a `"raw" CAN message type
<https://github.com/openxc/openxc-message-format#raw-can-message-format>`_. You
can configure the VI firmware to output raw CAN messages using this format.

For example, this JSON configuration will output all CAN messages received on a
high speed bus connecetd to the CAN1 controller:

.. code-block:: js

  {   "name": "passthrough",
      "buses": {
          "hs": {
              "controller": 1,
              "raw_can_mode": "unfiltered",
              "speed": 500000
          }
      }
  }

The only change from a typical configuration is the addition of the
``raw_can_mode`` attribute to the bus, set to ``unfiltered``. When using the raw
CAN configuration, there's no need to configure any messages or signals.

You may use both translated and raw output simultaneously - the 2 message types
will be interleaved on the output interfaces, so you'll need to check for the
right fields before reading the output.

If you're only interested in a few CAN messages, you can send a filtered set of
raw messages. Change the ``raw_can_mode`` to ``filtered`` and add the messages
ID's you want:

.. code-block:: js

  {   "name": "passthrough",
      "buses": {
          "hs": {
              "controller": 1,
              "raw_can_mode": "filtered",
              "speed": 500000
          }
      },
      "messages": {
        "0x21": {
          "bus": "hs"
        }
      }
  }

This will read and send the message with ID ``0x21`` only.

There are yet more ways to configure and control the low-level output (e.g.
limiting the data rate as to not overwhelm the VI's output channels) - see the
`code generation docs
<http://python.openxcplatform.com/en/latest/code-generation.html>`_ for complete
information.

Writing to CAN
==============

By default, the CAN controllers on the VI are configured to be in a read-only
mode - they won't even send ACK frames. If you configure one of the buses to be
``raw_writable`` in the firmware configuration, the controller will be
write-enabled for raw CAN messages, e.g.:

.. code-block:: js

  {   "name": "passthrough",
      "buses": {
          "hs": {
              "controller": 1,
              "raw_can_mode": "unfiltered",
              "raw_writable": true,
              "speed": 500000
          }
      }
  }

With a writable bus, you can send CAN messages (in the OpenXC "raw" message JSON
format) to the VI's input interfaces (e.g. USB, Bluetooth) and they'll be
written out to the bus verbatim.

Obviously this is an **advanced** feature with many security and safety
implications. The CAN controllers are configured as read-only by default
for good reason - make sure you understand the risks before enabling raw CAN
writes.

For additional security, by default the firmware will not accept raw CAN write
requests from remote interfaces even if ``raw_writable`` is true. Write requests
from Bluetooth and network connections will be ignored - only USB is allowed. I
you wish to write raw CAN messages wirelessly (and understand that those words
make security engineers queasy), compile with the ``ALLOW_REMOTE_RAW_WRITES``
flag (see :doc:`all compile-time flags </compiling>`).

The raw CAN write support is intended soley for protoyping and advanced
development work - for any sort of consumer-level app, it's much better to use
writable translated messages.
