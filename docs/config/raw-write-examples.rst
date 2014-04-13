========================================
Writable Raw CAN Configuration Examples
========================================

Writing raw CAN messages to the vehicle's bus is the lowest level you can get,
so please make sure you know what you're doing before trying any of these
examples. In addition to :doc:`reading raw CAN messages <raw-examples>` you can
write arbitrary CAN messages back to the bus.

.. contents::
    :local:
    :depth: 1

.. _raw-write-config:

Write a CAN messages
====================

There's not much to configure for raw CAN writes, since your application is
decided what message to build instead of the VI. The only requirement is that
the bus you wish to write on is configured as writable with the
``raw_writable`` flag, otherwise the VI will reject the request to send.

.. code-block:: js

  {   "buses": {
          "hs": {
              "controller": 1,
              "speed": 500000,
               "raw_writable": true
          }
      }
  }

With this configuration, you can send a CAN message write request to the VI
using the `OpenXC raw CAN message format
<https://github.com/openxc/openxc-message-format#raw-can-message-format>`_ (via
any of the I/O interfaces) and it will send it out to the bus. For example, when
using the JSON output format, sending this to the VI:

.. code-block:: js

  {"bus": 1, "id": 1234, "value": "0x12345678"}

would cause it to write a CAN message with the arbitration ID 1234 and the
payload 0x12345678 to the bus attached to the first CAN controller.

With the tools from the _`OpenXC Python library` you can send that from a
terminal with the command:

.. code-block:: sh

    openxc-control write --bus 1 --id 1234 ---data 0x12345678

and if you compiled the firmware with ``DEBUG=1``, you can view the view the
logs to make sure the write went through with this command:

.. code-block:: sh

    openxc-control write --bus 1 --id 1234 ---data 0x12345678 --log-mode stderr

Writing with Filtered CAN
=========================

It's also possible to write arbitrary CAN messages with you are using
:ref:`filtered raw CAN reads <filtered-raw-can>`. Your write requests aren't
restricted to the filtered set, simply add the ``raw_writable`` flag and you can
send any message.

.. code-block:: js

  {   "buses": {
          "hs": {
              "controller": 1,
              "speed": 500000,
              "raw_can_mode": "filtered",
               "raw_writable": true
          }
      },
      "messages": {
        "0x21": {
          "bus": "hs"
        }
      }
  }

The rest is the same as in the unfiltered write example.
