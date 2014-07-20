=====================================
Customizing the VI Configuration
=====================================

The open source VI firmware doesn't include any CAN
message definitions. If you know the details of your CAN signals, you can add
your own implementation of the functions defined in ``signals.h`` to a file
named ``signals.cpp``.

In this example, we'll pick a simple use case and walk through how to
configure and compile the firmware. You'll need to be comfortable
getting around at the command line, but you don't need to know any C++.
The VI firmware can be developed in Windows, Linux or Mac OS X.

Let's say we have a vehicle with a high speed CAN bus on the standard HS
pins, connecting to the "CAN1" controller on our vehicle interface.
There's a CAN message on this bus sent by an ECU, and we want to read
one numeric signal from the message - for this example, let it be the
accelerator pedal position as a percentage.

CAN Message and Signal Details
==============================

The message contains driver control signals, so we'll give it the name
``Driver_Controls`` so we can keep track of it. The message ID is
``0x102``.

In the message, there is a signal we'll call ``Accelerator_Pedal_Pos``
that starts at bit ``5`` and is ``7`` bits wide - enough to represent
pedal positions from 0 to 100.

The value on the bus is exactly how we want it to appear in the
translated version over USB or Bluetooth. We want the name to be
``accelerator_pedal_position`` and we want to hide the rest of the
details. We want the output format, sent via USB and UART (i.e. Bluetooth) from
the VI to be the standard translated OpenXC message format (see the `message
format specification <https://github.com/openxc/openxc-message-format>`_):

.. code-block:: js

   {"name": "accelerator_pedal_position", "value": 42}


JSON Configuration
==================

The configuration files for the VI firmware are stored as a single `JSON
<http://en.wikipedia.org/wiki/JSON>`_ object in a file. JSON is a human-readable
data format that's a alternative to XML - we use it because it's easy to parse,
(sorta) easy to write by hand and the syntax is fairly obvious.

CAN Bus Definition
------------------

We'll start by defining the CAN buses that we want to connect - open a file
called ``accelerator-config.json`` and add this:

.. code-block:: js

   {   "name": "accelerator",
       "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       }
   }

-  We gave this configuration the name ``accelerator`` - that will show
   up when we query for the query from the VI.
-  We defined 1 CAN bus and called it ``hs`` for "high speed" - the name
   is arbitrary but we'll use it later on, so make it short and sweet.
   ``hs``, ``ms``, ``info`` - these are good names.
-  We configured this bus to be connected to the ``#1`` controller on
   the VI - that's typically what's connected to the high speed bus in
   most vehicles.
-  We set the speed of this CAN bus at 500Kbps - the ``speed`` attribute
   is in bytes per second, so we set it to ``500000``.

CAN Message Definition
----------------------

Next up, let's define the CAN message we want to translate from the bus.
Modify the file so it looks like this:

.. code-block:: js

   {   "name": "accelerator",
       "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x102": {
               "name": "Driver_Controls",
               "bus": "hs"
           }
       }
   }

-  We added a ``messages`` field to the JSON object.
-  We added a ``0x102`` field to ``messages`` - that's our CAN message's
   ID and we use it here as a "key" for the object.
-  Within the ``0x102`` message object:
-  We set the name of the message. This is just used in comments so we
   can keep track of which message is which, rather than memorizing the
   ID.
-  We set the ``bus`` field to ``hs``, so this message will be pulled
   from the bus we defined (and named ``hs``).

CAN Signal Definition
---------------------

Don't stop yet...we have to define our CAN signal before anything will
be translated. Modify the file again:

.. code-block:: js

   {   "name": "accelerator",
       "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x102": {
               "name": "Driver_Controls",
               "bus": "hs",
               "signals": {
                   "Accelerator_Pedal_Pos": {
                       "generic_name": "accelerator_pedal_position",
                       "bit_position": 5,
                       "bit_size": 7
                   }
               }
           }
       }
   }

-  We added a ``signals`` field to the ``0x102`` message object, after
   the ``name``. The order doesn't matter, just watch out for the commas
   required after each field and value pair. There's no comma after the
   last field in an object.
-  We added an ``Accelerator_Pedal_Pos`` field in the ``signals`` object
   - that's the name of the signal, and like the message name, this is
   just for human readability.
-  The ``generic_name`` is what the ``name`` field will be in the
   translated format over USB and Bluetooth - we set it to
   ``accelerator_pedal_position``.
-  We set the ``bit_position`` and ``bit_size`` for the signal.

That's it - the configuration is finished. When we compile the VI
firmware with this configuration, it will read our CAN message from the
bus, parse and translate it into a JSON output message with a ``name``
and ``value``, and send it out over USB and Bluetooth. Next, we'll :doc:`walk
through how to do the compilation with your config </getting-started/with-config>`.
