#!/usr/bin/env bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd $DIR/..

KERNEL=`uname`
if [ ${KERNEL:0:7} == "MINGW32" ]; then
    OS="windows"
elif [ ${KERNEL:0:6} == "CYGWIN" ]; then
    OS="cygwin"
elif [ $KERNEL == "Darwin" ]; then
    OS="mac"
else
    OS="linux"
    if ! command -v lsb_release >/dev/null 2>&1; then
        # Arch Linux
        if command -v pacman>/dev/null 2>&1; then
            sudo pacman -S lsb-release
        fi
    fi

    DISTRO=`lsb_release -si`
fi

die() {
    echo >&2 "${bldred}$@${txtrst}"
    exit 1
}

_cygwin_error() {
    echo
    echo "${bldred}Missing \"$1\"${txtrst} - run the Cygwin installer again and select the base package set:"
    echo "    $CYGWIN_PACKAGES"
    echo "After installing the packages, re-run this bootstrap script."
    die
}

if ! command -v tput >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        echo "OPTIONAL: Install the \"ncurses\" package in Cygwin to get colored shell output"
    fi
else
    txtrst=$(tput sgr0) # reset
    bldred=${txtbld}$(tput setaf 1)
    bldgreen=${txtbld}$(tput setaf 2)
fi


_pushd() {
    pushd $1 > /dev/null
}

_popd() {
    popd > /dev/null
}

_wait() {
    if [ -z $CI ]; then
        echo "Press Enter when done"
        read
    fi
}

_install() {
    if [ $OS == "cygwin" ]; then
        _cygwin_error $1
    elif [ $OS == "mac" ]; then
        # brew exists with 1 if it's already installed
        set +e
        brew install $1
        set -e
    else
        if [ -z $DISTRO ]; then
            echo
            echo "Missing $1 - install it using your distro's package manager or build from source"
            _wait
        else
            if [ $DISTRO == "arch" ]; then
                sudo pacman -S $1
            elif [ $DISTRO == "Ubuntu" ]; then
                sudo apt-get update -qq
                sudo apt-get install $1 -y
            else
                echo
                echo "Missing $1 - install it using your distro's package manager or build from source"
                _wait
            fi
        fi
    fi
}

CYGWIN_PACKAGES="git, curl, libsasl2, ca-certificates, ncurses, python-setuptools"

download() {
    url=$1
    filename=$2
    curl $url -L --O $filename
}

if [ `id -u` == 0 ]; then
    die "Error: running as root - don't use 'sudo' with this script"
fi

if ! command -v curl >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "curl"
    else
        _install curl
    fi
fi

echo "Storing all downloaded dependencies in the \"dependencies\" folder"

DEPENDENCIES_FOLDER="dependencies"
mkdir -p $DEPENDENCIES_FOLDER

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
            sudo installer -pkg $FTDI_VOLUME/FTDIUSBSerialDriver_10_4_10_5_10_6_10_7.mpkg -target /
            hdiutil detach $FTDI_VOLUME
        fi
    fi
    _popd
fi


if [ -z "$MPIDE_DIR" ] || ! test -e $MPIDE_DIR || [ $OS == "cygwin" ]; then

    echo "Installing MPIDE to get avrdude for flashing chipKIT Max32 platform"

    if [ $OS == "cygwin" ]; then
        MPIDE_BASENAME="mpide-0023-windows-20120903"
        MPIDE_FILE="$MPIDE_BASENAME".zip
        EXTRACT_COMMAND="unzip -q"
        if ! command -v unzip >/dev/null 2>&1; then
            _cygwin_error "unzip"
        fi
    elif [ $OS == "mac" ]; then
        MPIDE_BASENAME=mpide-0023-macosx-20120903
        MPIDE_FILE="$MPIDE_BASENAME".dmg
    else
        MPIDE_BASENAME=mpide-0023-linux-20120903
        MPIDE_FILE="$MPIDE_BASENAME".tgz
        EXTRACT_COMMAND="tar -xzf"
    fi

    MPIDE_URL=https://github.com/downloads/chipKIT32/chipKIT32-MAX/$MPIDE_FILE

    _pushd $DEPENDENCIES_FOLDER
    if ! test -e $MPIDE_FILE
    then
        echo "Downloading MPIDE..."
        download $MPIDE_URL $MPIDE_FILE
    fi

    if ! test -d mpide
    then
        echo "Installing MPIDE to local folder..."
        if [ $OS == "mac" ]; then
            hdiutil attach $MPIDE_FILE
            cp -R /Volumes/Mpide/Mpide.app/Contents/Resources/Java $MPIDE_BASENAME
            hdiutil detach /Volumes/Mpide
        else
            $EXTRACT_COMMAND $MPIDE_FILE
        fi
        mv $MPIDE_BASENAME mpide
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

ARCH=`uname -m`
if [ $OS == "linux" ] && [ $ARCH == "x86_64" ]; then
    if [ $DISTRO == "arch" ]; then
        echo "Make sure lib32-libusb-compat and lib32-readline are installed from the AUR"
    elif [ $DISTRO == "Ubuntu" ]; then
        # TODO  figure out what is neccessary from a fresh build
        echo ""
    fi

fi

popd

echo
echo "${bldgreen}All mandatory dependencies installed, ready to flash.$txtrst"
