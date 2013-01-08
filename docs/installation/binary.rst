Pre-compiled Binary Firmware
=============================

Updates to the CAN translator firmware may be distributed as pre-compiled
binaries, e.g. if they are distributed by an OEM who does not wish to make the
CAN signals public. If that's the case for your vehicle, you will have a `.hex`
file and can use the [upload_hex.sh][] script to update your device.

You need to have the **mini-USB** port connected to your computer to upload a
new firmware. This is different than the USB port that you use to read vehicle
data - see the <a href="/vehicle-interface/index.html#connections">device
connections</a> section to make sure you have the correct cable attached.

The script requires that `avrdude` is installed. There are two possible ways to
get that dependency.

<div class="alert alert-info">
If you are using Windows or OS X, you also need to install the
<a href="http://www.ftdichip.com/Drivers/VCP.htm">FTDI driver</a> - get the
setup executable version from that page.
</div>

*Without MPIDE*

If you do not already have [MPIDE][] installed (and that's fine, you don't
really need it), you can find `avrdude` in your Linux distribution's package
manager, through Homebrew in Mac OS X, or as [WinAVR][winavr] in Windows.

*With MPIDE*

If you have [MPIDE][] installed, that already includes a version of avrdude. You
need to set the `MPIDE_DIR` environment variable in your terminal to point to
the folder where you installed MPIDE. Once set, you should be able to use
[upload_hex.sh][].

**Flashing**

Once you have `avrdude` installed, run the script with the `.hex` file you
downloaded (Windows users see below, this script will *not* work with the standard
Windows command line or Powershell):

{% highlight sh %}
$ sh ./upload_hex.sh <path to the .hex file you downloaded>
{% endhighlight %}

Windows users can use the GUI provided by [WinAVR][winavr] or run the
`upload_hex.sh` script with Cygwin. When using the script, you
will need to figure out which COM port the device shows up as - the script
default is `com4` but it may be different on your computer.

To specify use `com3` for example:

{% highlight sh %}
$ sh ./upload_hex.sh <path to the .hex file you downloaded> com3
{% endhighlight %}

In Windows, this command will only work in Cygwin, not the standard cmd.exe or
Powershell. If you have the `sh.exe` program installed by some other means (e.g.
you have Git installed in Windows) then it will actually work in Powershell.

If you get errors about `$'\r': command not found` then you Git configuration
added `CRLF` line endings and so you must run the script like this:

{% highlight sh %}
$ set -o igncr && export SHELLOPTS && sh ./upload_hex.sh <path to the .hex file you downloaded>
{% endhighlight %}

[winavr]: http://winavr.sourceforge.net/
[upload_hex.sh]: https://github.com/openxc/cantranslator/blob/master/upload_hex.sh
[cantranslator]: https://github.com/openxc/cantranslator
[MPIDE]: https://github.com/chipKIT32/chipKIT32-MAX/downloads
[testing]: /vehicle-interface/testing.html
[Cygwin]: http://www.cygwin.com/
