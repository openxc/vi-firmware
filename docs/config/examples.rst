===============================
Basic Configuration Examples
===============================

If you haven't created a custom firmware for the OpenXC VI yet, we recommend the
`getting started with custom data guide
<http://openxcplatform.com/firmware/custom-data-example.html>`_.

For all examples, the ``name`` field for message is optional but strongly
encouraged to help keep track of the mapping.

When an example refers to "sending" a translated or raw message, it means
sending to the app developer via one of the output interfaces (e.g. USB,
Bluetooth) and not sending to the CAN bus. For examples of configuring writable
messages and signals that *do* write back to the CAN bus, see the :doc:`write
configuration examples <write-examples>`.

.. contents::
    :local:
    :depth: 1

.. _onebus-onesignal:

One Bus, One Numeric Signal
==============================

We want to read a single, numeric signal from a high speed bus on controller 1.
The signal is 7 bits wide, starting from bit 5 in message ID ``0x102``. We want
the name of the signal for OpenXC app developers to be
``my_openxc_measurement``.

.. code-block:: sh

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
                       "bit_size": 7
                   }
               }
           }
       }
   }

.. _basic-transformed:

Transformed Numeric Signal
==========================

We want to read the same signal as in the :ref:`One Bus, One Numeric Signal
<onebus-onesignal>` example, but we want to transform the value with a factor
and offset before sending it to eh app developer. The value on CAN must be
multiplied by -1.0 and offset by 1400.


.. code-block:: sh

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
                       "offset": 1400
                   }
               }
           }
       }
   }

We added the ``factor`` and ``offset`` attributes to the signal.

One Bus, One Boolean Signal
===========================

We want to read a boolean signal from a high speed bus on controller 1.
The signal is 1 bits wide, starting from bit 32 in message ID ``0x103``. We want
the name of the signal for OpenXC app developers to be
``my_boolean_measurement``. Because it is a boolean type, the value will appear
as ``true`` or ``false`` in the JSON for app developers.

.. code-block:: sh

   {   "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x103": {
               "bus": "hs",
               "signals": {
                   "My_Boolean_Signal": {
                       "generic_name": "my_boolean_measurement",
                       "bit_position": 32,
                       "bit_size": 1,
                       "handler": "booleanHandler"
                   }
               }
           }
       }
   }

We set the ``handler`` for the signal to the ``booleanHandler``, one of the
:ref:`built-in signal handler functions <value-handlers>` - this will transform
the numeric value from the bus (a ``0`` or ``1``) into first-class boolean
values (``true`` or ``false``).

.. _state-based:

One Bus, One State-based Signal
===============================

We want to read a signal from a high speed bus on controller 1 that has numeric
values corresponding to a set of states - what we call a state-based signal

The signal is 3 bits wide, starting from bit 28 in message ID ``0x104``. We want
the name of the signal for OpenXC app developers to be
``active_state``. There are 6 valid states from 0-5, and we want those to
appears as the state strings ``a`` through ``f`` in the JSON for app developers.

.. code-block:: sh

   {   "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x104": {
               "bus": "hs",
               "signals": {
                   "My_State_Signal": {
                       "generic_name": "active_state",
                       "bit_position": 28,
                       "bit_size": 3,
                       "states": {
                           "a": [0],
                           "b": [1],
                           "c": [2],
                           "d": [3],
                           "e": [4],
                           "f": [5]
                       }
                   }
               }
           }
       }
   }

We set the ``states`` field for the signal to a JSON object mapping the string
value for each state to the numerical values to which it corresponds. This
automatically will set the ``handler`` to the ``stateHandler``, one of the
:ref:`built-in signal handler functions <value-handlers>`.

Combined State-based Signal
===========================

We want to read the same state-based signal from :ref:`state-based` but we want
the values 0-3 on the bus to all correspond with state ``a`` and values ``4-5``
to the string state ``b``.

.. code-block:: sh

   {   "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           }
       },
       "messages": {
           "0x104": {
               "bus": "hs",
               "signals": {
                   "My_State_Signal": {
                       "generic_name": "active_state",
                       "bit_position": 28,
                       "bit_size": 3,
                       "states": {
                           "a": [0, 1, 2, 3],
                           "b": [4, 5]
                       }
                   }
               }
           }
       }
   }

Each state string maps to an array - this can seem unnecessary when you only
have 1 numeric value for each state, but it allows combined mappings as in this
example.

Two Buses, Two Signals
======================

We want to read two numeric signals - one from a message on a high speed bus on
controller 1, and the other from a message on a medium speed bus on controller
2.

The signal on the high speed bus is 12 bits wide, starting from bit 11 in
message ID ``0x108``. We want the name of the signal for OpenXC app developers
to be ``my_first_measurement``.

The signal on the medium speed bus 14 bits wide, starting from bit 0 in message
ID ``0x90``. We want the name of the signal for OpenXC app developers to be
``my_second_measurement``.

.. code-block:: sh

   {   "buses": {
           "hs": {
               "controller": 1,
               "speed": 500000
           },
           "ms": {
               "controller": 2,
               "speed": 125000
           }
       },
       "messages": {
           "0x108": {
               "bus": "hs",
               "signals": {
                   "My_Signal": {
                       "generic_name": "my_first_measurement",
                       "bit_position": 11,
                       "bit_size": 12
                   }
               }
           },
           "0x90": {
               "bus": "ms",
               "signals": {
                   "My_Other_Signal": {
                       "generic_name": "my_second_measurement",
                       "bit_position": 0,
                       "bit_size": 14
                   }
               }
           }
       }
   }

We added the second bus to the ``buses`` field and assigned it to controller 2.
We added the second message object and made sure to set its ``bus`` field to
``ms``.

.. _limited-translated:

Limited Translated Signal Rate
==============================

We want to read the same signal as in the :ref:`One Bus, One Numeric Signal
<onebus-onesignal>` example, but we want it to be sent at a maximum of 5Hz. We
want the firmware to pick out messages at a regular period, but we don't care
which data is dropped in order to stay under the maximum.

.. code-block:: sh

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
                       "max_frequency": 5
                   }
               }
           }
       }
   }

We set the ``max_frequency`` field of the signal to 5 (meaning 5Hz) - the
firmware will automatically handle skipping messages to stay below this limit.

.. _limited-translated-unchanged:

Limited Translated Signal Rate if Unchanged
===========================================

We want the same signal from :ref:`limited-translated` at a limited rate, but we
don't want to lose any information - if the value of the signal changes, we want
it to be sent regardless of the max frequency. Repeated, duplicate signal values
are fairly common in vehicles, where a signal is sent at a steady frequency
even if the value hasn't changed. For this example, we want to preserve all
information - if a signal changes, we want to make sure the data is sent.

.. code-block:: sh

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
                       "max_frequency": 5,
                       "force_send_changed": true
                   }
               }
           }
       }
   }

We added the ``force_send_changed`` field to the signal, which will make sure
the signal is sent immediately when the value changes. This rate limiting is
lossless.

.. _send-on-change:

Send Signal on Change Only
===========================

We want to limit the rate of a signal as in :ref:`limited-translated-unchanged`,
but we want to be more strict - the signal should only be translated and sent to
app developers if it actually changes.

.. code-block:: sh

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
                       "send_same": false
                   }
               }
           }
       }
   }

We accomplish this by setting the ``send_same`` field to false. This is most
appropriate for boolean and state-based signals where the transition is most
important. Considering that a host device may connect to the VI *after* the
message has been sent, using this field has the potential of making it difficult
to tell the current state of the vehicle on startup - you have to wait for a
state change before knowing any values. For that reason, we've moved away from
using this for most firmware (using a combination of a ``max_frequency`` of 1Hz
and ``force_send_changed == true``) but the option is still available.
