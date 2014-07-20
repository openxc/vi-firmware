set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

if [ -z $CI ] && ! command -v openocd >/dev/null 2>&1; then

    ## Download OpenOCD for flashing ARM via JTAG
    _pushd $DEPENDENCIES_FOLDER

    echo "Installing OpenOCD..."
    if [ $OS == "linux" ]; then
        _install "openocd"
    elif [ $OS == "mac" ]; then
        _install libftdi
        _install libusb
        set +e
        brew install --enable-ft2232_libftdi open-ocd
        set -e
    elif [ $OS == "cygwin" ]; then
        echo
        echo "Missing OpenOCD and it's not trivial to install in Windows - you won't be able to program the ARM platform (not required for the chipKIT translator)"
    fi
    _popd
fi

FTDI_USB_DRIVER_PLIST=/System/Library/Extensions/FTDIUSBSerialDriver.kext/Contents/Info.plist
if [ $OS == "mac" ] && [ -e $FTDI_USB_DRIVER_PLIST ]; then
    if grep -q "Olimex OpenOCD JTAG A" $FTDI_USB_DRIVER_PLIST; then
        $SUDO_CMD sed -i "" -e "/Olimex OpenOCD JTAG A/{N;N;N;N;N;N;N;N;N;N;N;N;N;N;N;N;d;}" $FTDI_USB_DRIVER_PLIST
        FTDI_USB_DRIVER_MODULE=/System/Library/Extensions/FTDIUSBSerialDriver.kext/
        # Driver may not be loaded yet, but that's OK - don't exit on error.
        set +e
        $SUDO_CMD kextunload $FTDI_USB_DRIVER_MODULE
        set -e
        $SUDO_CMD kextload $FTDI_USB_DRIVER_MODULE
    fi
fi

echo
echo "${bldgreen}ARM JTAG flashing depenencies installed.$txtrst"
