## Installation

Building the source for the CAN translator for the chipKIT microcontroller
requires [MPIDE][] (the development environment and compiler toolchain for
chipKIT provided by Digilent). Installing MPIDE can be a bit quirky on some
platforms, so if you're having trouble take a look at the [installation guide
for MPIDE](http://chipkit.org/wiki/index.php?title=MPIDE_Installation).

Some of the library dependencies are included in this repository as git
submodules, so before you go further run:

    $ git submodule init && git submodule update

If this doesn't print out anything or gives you an error, make sure you cloned
this repository from GitHub with git and that you didn't download a zip file.
The zip file is missing all of the git metadata, so submodules will not work.

It also requires some libraries from Microchip that we are unfortunately unable
to include or link to as a submodule from the source because of licensing
issues:

* Microchip USB device library (download DSD-0000318 from
    the bottom of the [Network Shield page][])
* Microchip CAN library (included in the same DSD-0000318 package as the USB
  device library)

Download the zip file that contains these two libraries and extract the folders
into the `libs` directory in this project. It should look like this:

    - /Users/me/projects/cantranslator/
    ---- libs/
    -------- chipKITUSBDevice/
             chipKitCAN/
            ... other libraries

If you're using Mac OS X or Windows, make sure to install the FTDI driver that
comes with the MPIDE download. The chipKIT uses a different FTDI chip, so even
if you've used the Arduino before, you still need to install this driver.

[MPIDE]: https://github.com/chipKIT32/chipKIT32-MAX/downloads
[Max32 page]: http://digilentinc.com/Products/Detail.cfm?NavPath=2,719,895&Prod=CHIPKIT-MAX32
[Network Shield page]: http://digilentinc.com/Products/Detail.cfm?NavPath=2,719,943&Prod=CHIPKIT-NETWORK-SHIELD

Although we just installed MPIDE, building via the GUI is **not supported**. We
use GNU Make to compile and upload code to the device. You still need to
download and extract the MPIDE someplace, as it contains the PIC32 compilation
toolchain.

You need to set an environment variable (e.g. in your `.bashrc`) to let the
project know where you installed MPIDE (make sure to change these defaults if
your system is different!):

    # Path to the extracted MPIDE folder (this is correct for OS X)
    export MPIDE_DIR=/Applications/Mpide.app/Contents/Resources/Java

Remember that if you use `export`, the environment variables are only set in the
terminal that you run the commands. If you want them active in all terminals
(and you probably do), you need to add these `export ...` lines to the file
`~/.bashrc` (in Linux) or `~/.bash_profile` (in OS X) and start a new terminal.
