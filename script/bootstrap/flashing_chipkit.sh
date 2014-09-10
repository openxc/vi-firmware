set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

## FTDI library for programming chipKIT

if [ $OS == "cygwin" ] || [ $OS == "mac" ]; then

    if [ $OS == "cygwin" ]; then
        FTDI_DRIVER_FILE="DM20824_Setup.exe"
        FTDI_DRIVER_URL="http://www.ftdichip.com/Drivers/CDM/$FTDI_DRIVER_FILE"
        INSTALLED_FTDI_PATH="/cygdrive/c/Windows/System32/DriverStore/FileRepository"
        INSTALLED_FTDI_FILE="ftser2k.sys"
    elif [ $OS == "mac" ]; then
        FTDI_DRIVER_FILE="FTDIUSBSerialDriver_v2_2_18.dmg"
        FTDI_DRIVER_URL="http://www.ftdichip.com/Drivers/VCP/MacOSX/$FTDI_DRIVER_FILE"
        INSTALLED_FTDI_PATH=/System/Library/Extensions/FTDIUSBSerialDriver.kext/Contents/
        INSTALLED_FTDI_FILE=Info.plist
    fi

    _pushd $DEPENDENCIES_FOLDER
    if ! test -e $FTDI_DRIVER_FILE
    then
        echo "Downloading FTDI USB driver..."
        download $FTDI_DRIVER_URL $FTDI_DRIVER_FILE
    fi

    if [ -z "$(find $INSTALLED_FTDI_PATH -name $INSTALLED_FTDI_FILE | head -n 1)" ]; then

        if [ $OS == "cygwin" ]; then
            chmod a+x $FTDI_DRIVER_FILE
            cygstart.exe $FTDI_DRIVER_FILE
            echo -n "Press Enter when the FTDI USB driver installer is finished"
            read
        elif [ $OS == "mac" ]; then
            hdiutil attach $FTDI_DRIVER_FILE
            FTDI_VOLUME="/Volumes/FTDIUSBSerialDriver_v2_2_18"
            $SUDO_CMD installer -pkg $FTDI_VOLUME/FTDIUSBSerialDriver_10_4_10_5_10_6_10_7.mpkg -target /
            hdiutil detach $FTDI_VOLUME
        fi
    fi
    _popd
fi

if ! command -v unzip >/dev/null 2>&1; then
    _install "unzip"
fi

if [ -z "$MPIDE_DIR" ] || ! test -e $MPIDE_DIR || [ $OS == "cygwin" ]; then

    echo "Installing MPIDE to get avrdude for flashing chipKIT Max32 platform"

    MPIDE_BUILD=20140821
    if [ $OS == "cygwin" ]; then
        MPIDE_BASENAME="mpide-0023-windows-$MPIDE_BUILD"
        MPIDE_FILE="$MPIDE_BASENAME".zip
        EXTRACT_COMMAND="unzip -q"
    elif [ $OS == "mac" ]; then
        MPIDE_BASENAME=mpide-0023-macosx-$MPIDE_BUILD
        MPIDE_FILE="$MPIDE_BASENAME".dmg.zip
        EXTRACT_COMMAND="unzip -q"
    else
        MPIDE_BASENAME=mpide-0023-linux64-20140821
        MPIDE_FILE="$MPIDE_BASENAME".tgz
        EXTRACT_COMMAND="tar -xzf"
    fi

    MPIDE_URL=http://chipkit.s3.amazonaws.com/builds/$MPIDE_FILE

    _pushd $DEPENDENCIES_FOLDER
    if ! test -e $MPIDE_FILE
    then
        echo "Downloading MPIDE..."
        download $MPIDE_URL $MPIDE_FILE
    fi

    if ! test -d $MPIDE_BASENAME
    then
        echo "Installing MPIDE to local folder..."
        $EXTRACT_COMMAND $MPIDE_FILE
        if [ $OS == "mac" ]; then
            hdiutil attach $MPIDE_FILE
            cp -R /Volumes/Mpide/Mpide.app/Contents/Resources/Java $MPIDE_BASENAME
            hdiutil detach /Volumes/Mpide
        fi
        rm -rf mpide
        cp -R $MPIDE_BASENAME mpide
        echo "MPIDE installed"
    fi

    if [ $OS == "cygwin" ]; then
        chmod a+x mpide/hardware/pic32/compiler/pic32-tools/bin/*
        chmod a+x -R mpide/hardware/pic32/compiler/pic32-tools/pic32mx/
        chmod a+x mpide/*.dll
        chmod a+x mpide/hardware/tools/avr/bin/*
    fi
    _popd

fi

if [ $OS == "linux" ] && [ $ARCH == "x86_64" ]; then
    if [ $DISTRO == "arch" ]; then
        echo "Make sure lib32-libusb-compat and lib32-readline are installed from the AUR"
    elif [ $DISTRO == "Ubuntu" ]; then
        # TODO  figure out what is neccessary from a fresh build
        echo ""
    fi

fi

echo
echo "${bldgreen}chipKIT flashing dependencies installed.$txtrst"
