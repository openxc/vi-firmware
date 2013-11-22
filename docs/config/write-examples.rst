=====================================
Firmware Write Configuration Examples
=====================================

A "write" to the VI is an OpenXC formatted message sent in reverse - from an
application running on a host device back through e.g. USB or Bluetooth to the
VI. By default, any data sent back to the VI is ignored.

For applications that need to write data back to the CAN bus, whether for
actuation, personalization or diagnostics, you can configure a range of writing
styles to be permitted. At a high level, you can configure the VI to accept
writes for raw CAN messages, translated signals, translated signals with custom
logic on the VI to perform transformations, and completely arbitrary sets of CAN
writes from a single request from the user.

Most of these examples build on configurations started for reading data from the
bus, so you are strongly encouraged to read, understand and try the
:doc:`read-only configurations </config/examples>` before continuing.

.. contents::
    :local:
    :depth: 1

.. _translated-write:

Translated Numeric Signal Write Request
=======================================

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

Translated Boolean Signal Write Request
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
                       "write_handler": "booleanWriter"
                   }
               }
           }
       }
   }

In addition to setting ``writable`` to true, We set the ``write_handler`` for
the signal to the built-in ``booleanWriter``. This will handle converting a
``true`` or ``false`` value from the user back to a 1 or 0 in the outgoing CAN
message.

Translated State-based Signal Write Request
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
``write_handler`` because the signal has a ``states`` array. If a user sends the
VI the value ``c`` in a write request with the name ``my_state_request``, it
will be encoded as ``2`` in the CAN signal in the outgoing message.

Translated, Transformed Written Signal
=======================================

We want to write the same signal as :ref:`translated-write` but round any values
below 100 down to 0 before sending (similar to the read-only example
:ref:`custom-transformed`).

To accomplish this, we need to know a little C - we will write a custom signal
handler to make the transformation. Here's the JSON configuration:

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
                       "write_handler": "ourRoundingWriteHandler"
                   }
               }
           }
       },
       "extra_sources": [
         "my_handlers.cpp"
       ]
   }

We set the ``write_handler`` for the signal to ``ourRoundingWriteHandler``, and we'll
define that in a separate file named ``my_handlers.cpp``. The ``extra_sources``
field is also set, meaning that our custom C/C++ code will be included with the
firmware build.

In ``my_handlers.cpp``:

.. code-block:: cpp

   /* Round the value down to 0 if it's less than 100 before writing to CAN. */
   uint64_t ourRoundingWriteHandler(CanSignal* signal, CanSignal* signals,
        int signalCount, double value, bool* send) {
      if(value < 100) {
         value = 0;
      }
      // encodeSignal pulls the CAN signal definition from the CanSignal struct
      // and encodes the value into the right bits of a 64-bit return value.
      return encodeSignal(signal, value);
   }

Signal write handlers are responsible for encoding the value into a 64-bit
value, to be used in the outgoing message.

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

   bool handleMyCommand(const char* name, cJSON* value, cJSON* event,
         CanSignal* signals, int signalCount) {

      // Look up the numeric and boolean signals we need to send and abort if
      // either is missing
      CanSignal* numericSignal = lookupSignal("my_number_signal", signals,
            signalCount);
      CanSignal* booleanSignal = lookupSignal("my_value_is_over_100_signal",
            signals, signalCount);
      if(numericSignal == NULL) {
         debug("Unable to find numeric signal, can't send trio");
         // return false, indicating that we didn't successfully handle this
         // command
         return false;
      }

      // Send the arbitrary CAN message:

      // Build and enqueue the arbitrary CAN message to be sent - note that none
      // of the CAN messages we enqueue in the handler will be sent until after
      // it returns - interaction with the car via CAN must be asynchronous.
      CanMessage message = {0x34, 0x12345};
      // TODO need a lookupCanBus function to make sure we get the bus we wanted
      can::write::enqueueMessage(getCanBuses()[0], &message);

      // Send the numeric value:

      // The write API accepts cJSON objects right now as a way to accept
      // multiple types, so we create a cJSON number object wrapping the value
      // provided by the user
      cJSON* numberObject = cJSON_CreateNumber(value);
      can::write::sendSignal(numericSignal, numberObject, signals, signalCount,
              // the last parameter is true, meaning we want to force sending
              // this signal even though it's not marked writable in the
              // config
             true);
      // Make sure to free the cJSON object we created, otherwise it will leak
      // memory and quickly kill the VI
      cJSON_Delete(numberObject);

      // Send the boolean value:

      // Like above, create a cJSON object that wraps a boolean - true if the
      // value sent by the user is greater than 100
      cJSON* boolObject = cJSON_CreateBool(value > 100);
      // Send that boolean value in in the boolean signal on the bus, using the
      // booleanWriter write handler to convert it from a boolean to a number in
      // the message data
      can::write::sendSignal(booleanSignal, boolObject, booleanWriter,
              signals, signalCount,
              true);
      // again, make sure to free the cJSON object we created
      cJSON_Delete(boolObject);

      // we successfully processed the command, so return true to the VI stack
      return true;
   }
