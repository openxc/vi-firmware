Traces
=======

You can record a trace of JSON messages from the CAN reader for use in testing.
First, install the `OpenXC Python library`_. Then attach the CAN
translator to your computer via USB and use the `openxc-dump` program to print
all raw JSON messages to stdout. Redirect this to a file, and you've got your
trace. This can be used directly by the openxc-android library, for example.

    $ openxc-dump > vehicle-data.trace
