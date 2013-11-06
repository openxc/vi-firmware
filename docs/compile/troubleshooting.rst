===============
Troubleshooting
===============

If the compilation didn't work:

-  Make sure the submodules are up to date - run
   ``git submodule update --init`` and then ``git status`` and make sure
   there are no modified files in the working directory.
-  Did you download the .zip file of the ``vi-firmware`` project from
   GitHub? Use git to clone the repository instead - the library dependencies
   are stored as git submodules and do not work when using the zip file.
-  If you get a lot of errors about ``undefined reference to getSignals()'`` and
   other functions, you need to make sure you defined your CAN messages - read
   through :doc:`/config/config` before trying to compile.
