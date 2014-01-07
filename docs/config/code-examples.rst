=============================
Custom Firmware Code Examples
=============================

Sometimes a bit of C++ is required beyond the JSON configuration to implement
advanced features. You can optionally inject your own code into the generated
C++ file, so you can still take advantage of the configuration file for the
simple signals.

.. contents::
    :local:
    :depth: 1

.. _custom-transformed:

Custom Transformed Numeric Signal
=================================

Similar to the :ref:`basic-transformed` example, we want to modify a numeric
value read from a CAN signal before sending it to the app developer, but the
the desired transformation isn't as simple as an offset. We want to read the
same signal as before, but if it's below 100 it should be rounded down to 0. We
want our custom transformation to happen *after* using the existing factor and
offset.

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
                       "handler": "ourRoundingHandler"
                   }
               }
           }
       },
       "extra_sources": [
         "my_handlers.cpp"
       ]
   }

We set the ``handler`` for the signal to ``ourRoundingHandler``, and we'll
define that in a separate file named ``my_handlers.cpp``. We also added the
``extra_sources`` field, which is a list of the names of C++ source files on our
path to be included with the generated firmware code.

In ``my_handlers.cpp``:

.. code-block:: cpp

   /* Round the value down to 0 if it's less than 100. */
   float ourRoundingHandler(CanSignal* signal, CanSignal* signals,
         int signalCount, float value, bool* send) {
      if(value < 100) {
         value = 0;
      }
      return value;
   }

After being transformed with the factor and offset for the signal from the
configuration file, the value is passed to our handler function. We make
whatever custom transformation required and return the new value.

There are a few other valid type signatures for these :ref:`custom value
handlers <value-handlers>` - for converting numeric values to boolean or
state-based signals.

Transformed with Signal Reference
==================================

We need to combine the values of two signals from a CAN message to create a
single value - one signal is the absolute value, the other is the sign.

Both signals are on the high speed bus in the message with ID ``0x110``. The
absolute value signal is 5 bits wide, starting from bit 2. The sign signal is 1
bit wide, starting from bit 12 - when the value of the sign signal is 0, the
final value should be negative. We want to the final value to be sent to app
developers with the name ``my_signed_measurement``.

We will use a custom value handler for the signal to reference the sign
signal's last value when transforming the absolute value signal.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x110": {
               "bus": "hs",
               "signals": {
                   "My_Value_Signal": {
                       "generic_name": "my_signed_measurement",
                       "bit_position": 2,
                       "bit_size": 5,
                       "handler": "ourSigningHandler"
                   },
                   "My_Sign_Signal": {
                       "generic_name": "sign_of_signal",
                       "bit_position": 12,
                       "bit_size": 1,
                       "handler": "ignoreHandler"
                   }
               }
           }
       },
       "extra_sources": [
         "my_handlers.cpp"
       ]
   }

We don't want to the sign signal to be sent separately on the output interfaces,
but we need the firmware to read and store its value so we can refer to it from
our custom handler. We set the sign signal's ``handler`` to ``ignoreHandler``
which will still process and store the value, but withold it from the output
data stream.

For the absolute value signal, we set the ``handler`` to a custom function where
we look up the sign signal and use its value to transform the absolute value. In
``my_handlers.cpp``:

.. code-block:: cpp

   /* Load the last value for the sign signal and multiply the absolute value
   by it. */
   float ourRoundingHandler(CanSignal* signal, CanSignal* signals,
         int signalCount, float value, bool* send) {
       CanSignal* signSignal = lookupSignal("sign_of_signal",
               signals, signalCount);

       if(signSignal == NULL) {
           debug("Unable to find sign signal");
           *send = false;
       } else {
           if(signSignal->lastValue == 0) {
               // left turn
               value *= -1;
           }
       }
       return value;
   }

We use the `lookupSignal`` function to load a struct representing the
``sign_of_signal`` CAN signal we defined in the configuration, and check the
``lastValue`` attribute of the struct. If for some reason we aren't able to find
the configured sign signal, ``lookupSignal`` will return NULL and we can stop
hold the output of the final value by flipping ``*send`` to false. The firmware
will check the value of ``*send`` after each call to a custom handler to confirm
if the translation pipeline should continue.

One slight problem with this approach: there is currently no guaranteed
ordering for the signals. It's possible the ``lastValue`` for the sign signal
isn't from the same message as the absolute value signal you are current
handling in the function. With a continuous value, there's only a small window
where this could happen, but if you must be sure the values came from the same
message, you may need to write a :ref:`custom-message-handler`.

.. _custom-message-handler:

Composite Signal
================

We want complete control over the output of a measurement from the car. We have
a CAN message that includes 3 different signals that represent a GPS latitude
value, and want to combine them into a single value in degrees.

The three signals are in the message ``0x87`` on a high speed bus connected to
controller 1. The three signals:

- The whole latitude degrees signal starts at bit 10 and is 8 bits wide. The
  value on CAN requires an offset of -89.0.
- The latitude minutes signal starts at bit 18 and is 6 bits wide.
- The latitude minute fraction signal starts at bit 24 and is 14 bits wide. The
  value on CAN requires a factor of .0001.

.. code-block:: javascript

   {   "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x87": {
               "bus": "hs",
               "handler": "latitudeMessageHandler",
               "signals": {
                   "Latitude_Degrees": {
                       "generic_name": "latitude_degrees",
                       "bit_position": 10,
                       "bit_size": 8,
                       "offset": -89,
                       "ignore": true
                   },
                   "Latitude_Minutes": {
                       "generic_name": "latitude_minutes",
                       "bit_position": 18,
                       "bit_size": 6,
                       "ignore": true
                   },
                   "Latitude_Minute_Fraction": {
                       "generic_name": "latitude_minute_fraction",
                       "bit_position": 24,
                       "bit_size": 14,
                       "factor": 0.0001,
                       "ignore": true
                   },
               }
           }
       },
       "extra_sources": [
         "my_handlers.cpp"
       ]
   }

We made two changes to the configuration from a simple translation config:

- We set the ``ignore`` field to ``true`` for each of the component signals
  in the message. The signal definitions (i.e. the position, offset, etc) will
  be included in the firmware build so we can access it from a custom message
  handler, but the signals will not be processed by the normal translation
  stack.
- We set the ``handler`` for the ``0x87`` message (notice that unlike in other
  examples the ``handler`` is set on the message object in the config, not any
  of the signals) to our custom message handler, ``latitudeMessageHandler``.

In ``my_handlers.cpp``:

.. code-block:: cpp

    /* Combine latitude signals split into their components (degrees,
     * minutes and fractional minutes) into 1 output message: latitude in
     * degrees with with decimal precision.
     *
     * The following signals must be defined in the signal array, and they must
     * all be contained in the same CAN message:
     *
     *      * latitude_degrees
     *      * latitude_minutes
     *      * latitude_minutes_fraction
     *
     * This is a message handler, and takes care of sending the output message.
     *
     * messageId - The ID of the received GPS latitude CAN message.
     * data - The CAN message data containing all GPS latitude information.
     * signals - The list of all signals.
     * signalCount - The length of the signals array.
     * send - (output) Flip this to false if the message should not be sent.
     * pipeline - The pipeline that wraps the output devices.
     *
     * This type signature is required for all custom message handlers.
     */
    void latitudeMessageHandler(int messageId, uint64_t data,
            CanSignal* signals, int signalCount, Pipeline* pipeline) {
        // Retrieve the CanSignal struct representations of the 3 latitude
        // component signals. These are still included in the firmware build
        // when the 'ignore' flag was true for the signals.
        CanSignal* latitudeDegreesSignal =
            lookupSignal("latitude_degrees", signals, signalCount);
        CanSignal* latitudeMinutesSignal =
            lookupSignal("latitude_minutes", signals, signalCount);
        CanSignal* latitudeMinuteFractionSignal =
            lookupSignal("latitude_minute_fraction", signals, signalCount);

        // Confirm that we have all required signal components
        if(latitudeDegreesSignal == NULL ||
                latitudeMinutesSignal == NULL ||
                latitudeMinuteFractionSignal == NULL) {
            debug("One or more GPS latitude signals are missing");
            return;
        }

        // begin by assuming we will send the message, no errors yet
        bool send = true;

        // Decode and transform (using any factor and offset defined in the
        // CanSignal struct) each of the component signals from the message data
        // preTranslate is intended to be used in conjunction with postTranslate
        // - together they keep metadata about the receive signals in memory.
        float latitudeDegrees = preTranslate(latitudeDegreesSignal, data, &send);
        float latitudeMinutes = preTranslate(latitudeMinutesSignal, data, &send);
        float latitudeMinuteFraction = preTranslate(
                latitudeMinuteFractionSignal, data, &send);

        // if we were able to decode all 3 component signals (i.e. none of the
        // calls to preTranslate flipped 'send' to false
        if(send) {
            float latitude = (latitudeMinutes + latitudeMinuteFraction) / 60.0;
            if(latitudeDegrees < 0) {
                latitude *= -1;
            }
            latitude += latitudeDegrees;

            // Send the final latitude value to the output interfaces (via the
            // pipeline)
            sendNumericalMessage("latitude", latitude, pipeline);
        }

        // Conclude by updating the metadata for each of the component signals
        // with postTranslate
        postTranslate(latitudeDegreesSignal, latitudeDegrees);
        postTranslate(latitudeMinutesSignal, latitudeMinutes);
        postTranslate(latitudeMinuteFractionSignal, latitudeMinuteFraction);
    }

A more complete, functional example of a message handler is included in the VI
firmware repository - one that handles `both latitude and longitude in a CAN
message
<https://github.com/openxc/vi-firmware/blob/master/src/shared_handlers.h#L204>`_.
There is also additional documentation on the :ref:`message handler type
signature <message-handlers>`.

.. _initializer-example:

Initializer Function
=====================

We want to initialize a counter when the VI powers up that we will use from some
custom CAN signal handlers.

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
       },
       "initializers": [
          "initializeMyCounter"
       ],
       "extra_sources": [
         "my_initializers.cpp"
       ]
   }

We added an ``initializers`` field, which is an array containing the names of
C functions matching the :ref:`initializer type signature <initializer>`.

In ``my_initializers.cpp``:

.. code-block:: cpp

   int MY_COUNTER;
   void initializeMyCounter() {
      MY_COUNTER = 42;
   }

This isn't a very useful initializer, but there much more you could do - you'll
want to look into the lowest level APIs in the `firmware source
<https://github.com/openxc/vi-firmware>`_. Look through the ``.h`` files, where
most functions are documented.

.. _looper-example:

Looper Function
================

We want to increment a counter every time through the main loop of the firmware,
regardless of whatever CAN messages we may have received.

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
       },
       "loopers": [
          "incrementMyCounter"
       ],
       "extra_sources": [
         "my_loopers.cpp"
       ]
   }

We added a ``loopers`` field, which is an array containing the names of
C functions matching the :ref:`looper type signature <looper>`.

In ``my_loopers.cpp``:

.. code-block:: cpp

   void incrementMyCounter() {
      static int myCounter = 0;
      ++myCounter;
   }

As with the :ref:`initializer <initializer-example>`, this isn't a very
functional example, but there much more you could do - you'll want to look into
the lowest level APIs in the `firmware source
<https://github.com/openxc/vi-firmware>`_. Look through the ``.h`` files, where
most functions are documented.

Ignore Depending on Value
==========================

We want to ignore a signal (and not translate it and send over USB/Bluetooth) if
the value matches certain critera. We'll use a custom handler as in
:ref:`custom-transformed` but instead of modifying the value of the signal,
we'll use the ``send`` flag to tell the VI if it should process the value or
not.

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
                       "handler": "ourFilteringHandler"
                   }
               }
           }
       },
       "extra_sources": [
         "my_handlers.cpp"
       ]
   }

In ``my_handlers.cpp``:

.. code-block:: cpp

   /* Ignore the signal if the value is less than 100 */
   float ourFilteringHandler(CanSignal* signal, CanSignal* signals,
         int signalCount, float value, bool* send) {
      if(value < 100) {
         *send = false;
      }
      return value;
   }
