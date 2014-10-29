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

You may use both simple vehicle messages and CAN message output simultaneously -
the 2 message types will be interleaved on the output interfaces, so you'll need
to check for the right fields before reading the output.

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

The ``raw_can_mode`` flag can be applied to all active CAN buses at once by
defining it at the top level of the configuration. For example, this
configuration will enable unfiltered raw CAN output from 2 buses simultaneously:

.. code-block:: js

  {   "name": "passthrough",
      "raw_can_mode": "filtered",
      "buses": {
          "hs": {
              "controller": 1,
              "speed": 500000
          },
          "ms": {
              "controller": 2,
              "speed": 125000
          }
      }
  }

When defined at the top level, the ``raw_can_mode`` can be overridden by any of
the individual buses (e.g. ``hs`` could inherit the ``unfiltered`` setting but
``ms`` could override and set it to ``filtered``).

There are yet more ways to configure and control the low-level output (e.g.
limiting the data rate as to not overwhelm the VI's output channels) - see the
:ref:`raw configuration examples <unfiltered-raw>` for more information.

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
from Bluetooth and network connections will be ignored - only USB is allowed by
default. If you wish to write raw CAN messages wirelessly (and understand that
those words make security engineers queasy), compile with the
``NETWORK_ALLOW_RAW_WRITE`` or ``BLUETOOTH_ALLOW_RAW_WRITE`` flags (see
:doc:`all compile-time flags </compile/makefile-opts>`).

The raw CAN write support is intended soley for protoyping and advanced
development work - for any sort of consumer-level app, it's much better to use
writable simple vehicle messages.
