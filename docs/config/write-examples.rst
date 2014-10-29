===============================
Writable Configuration Examples
===============================

For applications that need to send data back to the vehicle, you can configure a
variety of CAN writes: raw CAN messages, performing reverse translation of an
individual CAN signal, or send diagnostic requests.

Most of these examples build on configurations started for reading data from the
bus, so you are strongly encouraged to read, understand and try the
:doc:`read-only configurations </config/examples>` before continuing.

.. contents::
    :local:
    :depth: 1

.. _simple-write:

Named Numeric Signal Write Request
==================================

We want to send a single numeric value to the VI, and have it translated back
into a CAN signal in a message on a high speed bus attached to controller 1. The
signal is 7 bits wide, starting from bit 5 in message ID ``0x102``. We want the
name of the signal that will be sent to the VI to be ``my_openxc_measurement``.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
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
                       "bit_size": 7,
                       "writable": true
                   }
               }
           }
       }
   }

This configuration is the same as the read-only example :ref:`onebus-onesignal`
with the addition of the ``writable`` flag to the signal. When this flag is
true, an OpenXC message sent back to the VI from an app with the name
``my_openxc_measurement`` will be translated to a CAN signal in a new message
and written to the bus.

If the VI is configured to use the JSON output format, sending this `OpenXC
single-valued message
<https://github.com/openxc/openxc-message-format#single-valued>`_ to the VI via
USB or UART (Bluetooth) would trigger a CAN write:

.. code-block:: js

   {"name": "my_openxc_measurement", "value": 42}

With the tools from the `OpenXC Python library <openxc-python>`_ you can send that from a
terminal with the command:

.. _openxc-python: http://python.openxcplatform.com/en/latest/

.. code-block:: sh

    openxc-control write --name my_openxc_measurement --value 42

and if you compiled the firmware with ``DEBUG=1``, you can view the view the
logs to make sure the write went through with this command:

.. code-block:: sh

    openxc-control write --name my_openxc_measurement --value 42 --log-mode stderr

Named Boolean Signal Write Request
=======================================

We want to send a single boolean value to the VI, and have it translated back
into a CAN signal in a message on a high speed bus attached to controller 1. The
signal is 1 bits wide, starting from bit 3 in message ID ``0x201``. We want the
name of the signal that will be sent to the VI to be ``my_boolean_request``.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x102": {
               "bus": "hs",
               "signals": {
                   "My_Signal": {
                       "generic_name": "my_boolean_request",
                       "bit_position": 3,
                       "bit_size": 1,
                       "writable": true,
                       "encoder": "booleanWriter"
                   }
               }
           }
       }
   }

In addition to setting ``writable`` to true, We set the ``encoder`` for
the signal to the built-in ``booleanWriter``. This will handle converting a
``true`` or ``false`` value from the user back to a 1 or 0 in the outgoing CAN
message.

If the VI is configured to use the JSON output format, sending this `OpenXC
single-valued message
<https://github.com/openxc/openxc-message-format#single-valued>`_ to the VI via
USB or UART (Bluetooth) would trigger a CAN write:

.. code-block:: js

   {"name": "my_boolean_request", "value": true}

With the tools from the `OpenXC Python library <openxc-python>`_ you can send
that from a terminal with the command:

.. code-block:: sh

    openxc-control write --name my_boolean_request --value true

Named State-based Signal Write Request
===========================================

We want to send a state as a string to the VI, and have it translated back into
a numeric CAN signal in a message on a high speed bus attached to controller 1.
As in :ref:`state-based`, the signal is 3 bits wide, starting from bit 28 in
message ID ``0x104``. We want the name of the signal for OpenXC app developers
to be ``active_state``. There are 6 valid states from 0-5 in the CAN signal, but
we want the app developer to send the strings ``a`` through ``f`` to the VI.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x102": {
               "bus": "hs",
               "signals": {
                   "My_Signal": {
                       "generic_name": "my_state_request",
                       "bit_position": 28,
                       "bit_size": 3,
                       "states": {
                           "a": [0],
                           "b": [1],
                           "c": [2],
                           "d": [3],
                           "e": [4],
                           "f": [5]
                       },
                       "writable": true
                   }
               }
           }
       }
   }

The ``writable`` field is all that is required - the signal will be
automatically configured to use the built-in ``stateWriter`` as its
``encoder`` because the signal has a ``states`` array. If a user sends the
VI the value ``c`` in a write request with the name ``my_state_request``, it
will be encoded as ``2`` in the CAN signal in the outgoing message.

If the VI is configured to use the JSON output format, sending this `OpenXC
single-valued message
<https://github.com/openxc/openxc-message-format#single-valued>`_ to the VI via
USB or UART (Bluetooth) would trigger a CAN write:

.. code-block:: js

   {"name": "my_state_request", "value": "a"}

With the tools from the `OpenXC Python library <openxc-python>`_ you can send that from a
terminal with the command:

.. code-block:: sh

    openxc-control write --name my_state_request --value "\"a\""

Becuase of the way string escaping works from the command prompt, you have to
add escaped ``\"`` characters so the tool knows you want to send a string.

Named, Transformed Written Signal
=======================================

We want to write the same signal as :ref:`simple-write` but round any values
below 100 down to 0 before sending (similar to the read-only example
:ref:`custom-transformed`).

To accomplish this, we need to know a little C - we will write a custom signal
encoder to make the transformation. Here's the JSON configuration:

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
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
                       "bit_size": 7,
                       "factor": -1.0,
                       "offset": 1400,
                       "encoder": "ourRoundingWriteEncoder"
                   }
               }
           }
       },
       "extra_sources": [
         "my_handlers.cpp"
       ]
   }

We set the ``encoder`` for the signal to ``ourRoundingWriteEncoder``, and we'll
define that in a separate file named ``my_handlers.cpp``. The ``extra_sources``
field is also set, meaning that our custom C/C++ code will be included with the
firmware build.

In ``my_handlers.cpp``:

.. code-block:: cpp

   /* Round the value down to 0 if it's less than 100 before writing to CAN. */
   uint64_t ourRoundingWriteEncoder(CanSignal* signal, CanSignal* signals,
        int signalCount, double value, bool* send) {
      if(value < 100) {
         value = 0;
      }
      // encodeSignal pulls the CAN signal definition from the CanSignal struct
      // and encodes the value into the right bits of a 64-bit return value.
      return encodeSignal(signal, value);
   }

Signal encoders are responsible for transforming a float, string or boolean
value into a 64-bit integer, to be used in the outgoing message.

.. _command-example:

Composite Write Request
=======================

When the app developer sends a numeric measurement to the VI, we want to send:

- 1 arbitrary CAN message with the ID ``0x34`` on a high speed bus connected to
  controller 1, with the value ``0x1234``.
- The value sent by the developer encoded into the message ID ``0x35`` in a
  signal starting at bit 0, 4 bits wide on the same high speed bus. We don't
  want this value to be writable by the app developer unless a part of these 3
  writes combined.
- A boolean signal in the message ``0x101`` on a medium speed bus connected to
  controller 2, starting at bit 12 and 1 bit wide. If the numeric value from the
  user is greater than 100, the boolean value should be ``true``.

.. code-block:: js

  {   "name": "passthrough",
      "buses": {
          "hs": {
              "controller": 1,
              "raw_writable": true,
              "speed": 500000
           },
           "ms": {
              "controller": 2,
              "speed": 125000
           }
      },
      "messages": {
          "0x35": {
              "bus": "hs",
              "signals": {
                  "My_Numeric_Signal": {
                      "generic_name": "my_number_signal",
                      "bit_position": 0,
                      "bit_size": 4
                  }
              }
          }
          "0x101": {
              "bus": "ms",
              "signals": {
                  "My_Other_Signal": {
                      "generic_name": "my_value_is_over_100_signal",
                      "bit_position": 12,
                      "bit_size": 1
                  }
              }
          }
      },
      "commands": [
         {"name": "my_command",
            "handler": "handleMyCommand"}
      ],
      "extra_sources": [
        "my_handlers.cpp"
      ]
  }

We added a ``commands`` field, which contains an array of JSON objects with
``name`` and ``handler`` fields. The name of the command, ``my_command`` is what
app developers will send to the VI. The ``handler`` is the name of a C++
function will define in one of the files listed in ``extra_sources``.

In the configuration, also note that:

- The raw CAN message that we want to send isn't included. Since
  ``raw_writable`` is true for the ``hs`` bus, there's no need to define it in
  the configuration.
- The ``my_number_signal`` signal doesn't have the ``writable`` flag set to true (it's
  omitted, and the default is ``false``). This means an app developer will not
  be able to send write requests for ``my_number_signal`` directly.

In ``my_handlers.cpp``:

.. code-block:: cpp

   void handleMyCommand(const char* name, openxc_DynamicField* value,
         openxc_DynamicField* event, CanSignal* signals, int signalCount) {

      // Look up the numeric and boolean signals we need to send and abort if
      // either is missing
      CanSignal* numericSignal = lookupSignal("my_number_signal", signals,
            signalCount);
      CanSignal* booleanSignal = lookupSignal("my_value_is_over_100_signal",
            signals, signalCount);
      if(numericSignal == NULL || booleanSignal == NULL) {
         debug("Unable to find signals, can't send trio");
         return;
      }

      // Build and enqueue the arbitrary CAN message to be sent - note that none
      // of the CAN messages we enqueue in the handler will be sent until after
      // it returns - interaction with the car via CAN must be asynchronous.
      CanMessage message = {0x34, 0x12345};
      CanBus* bus = lookupBus(0, getCanBuses(), getCanBusCount());
      if(bus != NULL) {
        can::write::enqueueMessage(bus, &message);
      }

      // Send the numeric signal
      can::write::encodeAndSendSignal(numericSignal, value,
             // the last parameter is true, meaning we want to force sending
             // this signal even though it's not marked writable in the
             // config
             true);

      // For the boolean signal, send 'true' if the value sent by the user is
      // greater than 100
      can::write::encodeAndSendBooleanSignal(booleanSignal, value > 100, true);
   }
