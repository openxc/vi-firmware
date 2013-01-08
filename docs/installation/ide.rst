IDE Support
-----------

It is possible to use an IDE like Eclipse to edit and compile the
project.

-  Follow the directions in the above "Installation" section.
-  Install Eclipse with the `CDT project <http://www.eclipse.org/cdt/>`_
-  In Eclipse, go to
   ``File -> Import -> C/C++ -> Existing Code as Makefile Project`` and
   then select the ``cantranslator/src`` folder.
-  In the project's properties, under
   ``C/C++ General -> Paths and Symbols``, add these to the include
   paths for C and C++:

   -  ``${MPIDE_DIR}/hardware/pic32/compiler/pic32-tools/pic32mx/include``
   -  ``${MPIDE_DIR}/hardware/pic32/cores/pic32``
   -  ``/cantranslator/libs/CDL/LPC17xxLib/inc`` (add as a "workspace
      path")
   -  ``/cantranslator/libs/chipKITCAN`` (add as a "workspace path")
   -  ``/cantranslator/libs/chipKITUSBDevice`` (add as a "workspace
      path")
   -  ``/cantranslator/libs/chipKITUSBDevice/utility`` (add as a
      "workspace path")
   -  ``/cantranslator/libs/chipKITEthernet`` (add as a "workspace
      path")
   -  ``/usr/include`` (only if you want to use the test suite, which
      requires the ``check`` C library)

-  In the same section under Symbols, if you are building for the
   chipKIT define a symbol with the name ``__PIC32__``
-  In the project folder listing, select
   ``Resource Configurations -> Exclude from   Build`` for these
   folders:

   -  ``libs``
   -  ``build``

If you didn't set up the environment variables from the ``Installation``
section (e.g. ``MPIDE_HOME``), you can also do that from within Eclipse
in ``C/C++`` project settings.

There will still be some errors in the Eclipse problem detection, e.g.
it doesn't seem to pick up on the GCC ``__builtin_*`` functions, and
some of the chipKIT libraries are finicky. This won't have an effect on
the actual build process, just the error reporting.
