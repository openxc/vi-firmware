====
FAQ
====

While building custom firmware, if you find yourself thinking:

- "I need to run some code in the main loop, regardless of any CAN signals." -
  use a :ref:`looper`.
- "I need to run initialization code at startup." User an :ref:`initializer`.
- "I need to do some extra processing on a CAN signal before sending it out."
  Use a :ref:`signal-handler`.
- "I need to combine the value of multiple signals in a message to generate a
  value." Use a :ref:`message-handler`.
- "I need to send less data." Control the :ref:`send-frequency`.
- "I don't want this signal to send unless it changes." Configure it to
  :ref:`send-on-change`.

.. _looper:

Looper
=======

.. _initializer:

Initializer
===========

.. _signal-handler:

Signal Handler
==============

.. _message-handler:

Message Handler
================

.. _send-frequency:

Send Frequency
==============

.. _send-on-change:

Send on Change
==============
