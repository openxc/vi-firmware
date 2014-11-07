========================================
Read-only Raw CAN Configuration Examples
========================================

If you don't care about abstracting the details of CAN messages from developers
(or perhaps you're the only developer and you're working directly with CAN
data), you can configure the VI to output full, low-level CAN messages. You can
read every message or a filtered subset, but be aware that not every VI has
enough horsepower to send every CAN messages through as JSON.

.. contents::
    :local:
    :depth: 1

.. _unfiltered-raw:

Unfiltered Raw CAN
==================

We want to read all raw CAN messages from a bus at full speed. Be aware that the
VI hasn't been optimized for this level of throughput, and it's not guaranteed
at this time that messages will not be dropped. We recommend using rate
limiting, which can dramatically decrease the bandwidth required without losing
any information.

.. code-block:: js

  {   "buses": {
          "hs": {
              "controller": 1,
              "speed": 500000,
              "raw_can_mode": "unfiltered"
          }
      }
  }

With this configuration, the VI will publish all CAN messages it receives using
the `OpenXC raw CAN message format
<https://github.com/openxc/openxc-message-format#raw-can-message-format>`_,
e.g. when using the JSON output format:

.. code-block:: js

  {"bus": 1, "id": 1234, "value": "0x12345678"}

.. _filtered-raw-can:

Filtered Raw CAN
=================

We want to read only the message with ID ``0x21`` from a high speed bus on
controller 1.

.. code-block:: js

  {   "buses": {
          "hs": {
              "controller": 1,
              "speed": 500000,
              "raw_can_mode": "filtered"
          }
      },
      "messages": {
        "0x21": {
          "bus": "hs"
        }
      }
  }

We added the ``0x21`` message and assigned it to bus ``hs``, but didn't define
any signals (it's not necessary when using the raw CAN mode).

This configuration will cause the VI to publish using the
`OpenXC raw CAN message format
<https://github.com/openxc/openxc-message-format#raw-can-message-format>`_, but
you will only received message ``0x21``.

.. _unfiltered-limited:

Unfiltered Raw CAN with Limited, Variable Data Rate
===================================================

We want to read all raw CAN messages from a bus, but we don't want the output
interface to be overwhelmed by repeated duplicate messages. This is fairly
common in vehicles, where a message is sent at a steady frequency even if the
value hasn't changed. For this example, we want to preserve all information - if
a message changes, we want to make sure the data is sent.

.. code-block:: js

  {   "buses": {
          "hs": {
              "controller": 1,
              "speed": 500000,
              "raw_can_mode": "unfiltered",
              "max_message_frequency": 1,
              "force_send_changed": true
          }
      }
  }

We combine two attributes to both limit the data rate from raw CAN messages, and
also make sure the transfer is lossless. The ``max_message_frequency`` field
sets the maximum send frequency for CAN messages that have not changed to 1Hz.
We also set the ``force_send_changed`` field to ``true``, which will cause a CAN
message with a new value to be sent to the output interface immediately, even if
it would go above the 1Hz frequency. The default is ``true``, so we could also
leave this parameter out for the same effect. The result is that each CAN
message is sent at a minimum of 1Hz and a maximum of the true rate of change for
the message.

Unfiltered Raw CAN with Strict, Limited Data Rate
=================================================

We want to read all raw CAN messages as in :ref:`unfiltered-limited` but we want
to set a strict limit on the read frequency of each CAN message. We don't care
if we skip some CAN messages, even if they have new data - the maximum frequency
is the most important thing.

.. code-block:: js

  {   "buses": {
          "hs": {
              "controller": 1,
              "speed": 500000,
              "raw_can_mode": "unfiltered",
              "max_message_frequency": 1,
              "force_send_changed": false.
          }
      }
  }

We set the ``force_send_changed`` field to false so the firmware will strictly
enforce the max message frequency.

Simple Messages and CAN Messages Together
=========================================

We want to read the same signal as in the :ref:`One Bus, One Numeric Signal
<onebus-onesignal>` example, but we also want to receive all unfiltered CAN
messages simultaneously.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "raw_can_mode": "unfiltered",
               "speed": 500000
           }
       },
       "messages": {
           "0x102": {
               "bus": "hs",
               "signals": {
                   "My_Signal": {
                       "generic_name": "my_openxc_measurement",
                       "bit_position": 5,
                       "bit_size": 7
                   }
               }
           }
       }
   }

We added set the ``raw_can_mode`` for the bus to ``unfiltered``, as in
:ref:`unfiltered-raw`. No other changes are required - the CAN and simple
vehicle messages
co-exist peacefully. If we set ``raw_can_mode`` to ``filtered``, it
would only send the raw message for ``0x102``, where we're getting the numeric
signal.

With this configuration, the VI will publish a mixed stream of OpenXC messages,
both the `CAN message format
<https://github.com/openxc/openxc-message-format/blob/next/JSON.mkd#can-message>`_,
and the `simple vehicle message format
<https://github.com/openxc/openxc-message-format/blob/next/JSON.mkd#simple-vehicle-message>`_,
e.g. when using the JSON output format:

.. code-block:: js

   {"bus": 1, "id": 258, "value": "0x12345678"}
   {"name": "my_openxc_measurement", "value": 42}
