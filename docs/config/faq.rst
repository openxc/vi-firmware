====
FAQ
====

While building custom firmware, if you find yourself thinking:

- "I need to run some code in the main loop, regardless of any CAN signals." -
  use a :ref:`looper-example`.
- "I need to run initialization code at startup." User an
  :ref:`initializer-example`.
- "I need to do some extra processing on a CAN signal before sending it out."
  Use a :ref:`custom signal decoder <custom-transformed>`.
- "I need to combine the value of multiple signals in a message to generate a
  value." Use a :ref:`message handler <custom-message-handler>`.
- "I need to send less data." Control the :ref:`send frequency
  <limited-simple>`.
- "I don't want this signal to send unless it changes." Configure it to
  :ref:`send-on-change`.
