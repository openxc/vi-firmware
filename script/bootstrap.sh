#!/usr/bin/env bash

set -e

die() {
    echo >&2 "$@"
    exit 1
}

KERNEL=`uname`
if [ ${KERNEL:0:7} == "MINGW32" ]; then
    die "Sorry, the bootstrap script doesn't support Windows - try Cygwin."
elif [ ${KERNEL:0:6} == "CYGWIN" ]; then
    OS="cygwin"
elif [ $KERNEL == "Darwin" ]; then
    OS="mac"
else
    OS="linux"
fi

# Grab libraries stored as git submodules

git submodule update --init

# Store all downloaded dependencies in one place

DEPENDENCIES_FOLDER="dependencies"
mkdir -p $DEPENDENCIES_FOLDER

# chipKIT Max32 Dependencies

if [ -z "$MPIDE_DIR"] || ! test -e $MPIDE_DIR; then

    ## MPIDE, which includes the PIC32 compiler

    if [ $OS == "cgywin" ]; then
        MPIDE_BASENAME="mpide-0023-windows-20120903"
        MPIDE_FILE="$MPIDE_BASENAME".zip
    elif [ $OS == "mac" ]; then
        # TODO how can we install this locally to the mpide directory? alternatively,
        # what if we just used the linux version in OS X?
        MPIDE_BASENAME=mpide-0023-macosx-20120903
        MPIDE_FILE="$MPIDE_BASENAME".dmg
    else
        MPIDE_BASENAME=mpide-0023-linux-20120903
        MPIDE_FILE="$MPIDE_BASENAME".tgz
    fi

    MPIDE_URL=https://github.com/downloads/chipKIT32/chipKIT32-MAX/$MPIDE_FILE

    pushd $DEPENDENCIES_FOLDER
    if ! test -e $MPIDE_FILE
    then
        wget $MPIDE_URL
    fi

    if ! test -d mpide
    then
        tar -xzf $MPIDE_FILE
        mv $MPIDE_BASENAME mpide
    fi
    popd

fi

## chipKIT libraries for USB, CAN and Ethernet

CHIPKIT_LIBRARY_AGREEMENT_URL="http://www.digilentinc.com/Agreement.cfm?DocID=DSD-0000318"
CHIPKIT_LIBRARY_DOWNLOAD_URL="http://www.digilentinc.com/Data/Documents/Product%20Documentation/chipKIT%20Network%20and%20USB%20Libs.zip"

pushd $DEPENDENCIES_FOLDER
if ! test -e chipkit.zip
then
    echo "By running this command, you agree to Microchip's licensing agreement at $CHIPKIT_LIBRARY_AGREEMENT_URL"
    wget $CHIPKIT_LIBRARY_DOWNLOAD_URL -O chipkit.zip
fi
popd

pushd src/libs
for LIBRARY in chipKITUSBDevice chipKITCAN chipKITEthernet; do
    if ! test -d $LIBRARY
    then
        unzip ../../dependencies/chipkit.zip "$LIBRARY/*"
    fi
done
popd

### Patch libraries to avoid problems in case sensitive operating systems
### See https://github.com/chipKIT32/chipKIT32-MAX/issues/146
### and https://github.com/chipKIT32/chipKIT32-MAX/issues/199

# If the patch is already applied, patch will error out, so disable quit on
# error temporarily
set +e
pushd src/libs
pushd chipKITUSBDevice
patch -p1 -sNi ../../../script/chipKITUSBDevice-case.patch
popd

pushd chipKITCAN
patch -p1 -sNi ../../../script/chipKITCAN-case.patch
popd

popd
set -e

# ARM / LPC17XX Dependencies

if test ! $(which arm-none-eabi-gcc); then

    ## Download GCC compiler for ARM Embedded

    GCC_ARM_ZIP="gcc-arm-none-eabi-4_6-2012q2-20120614.tar.bz2"
    GCC_ARM_URL="https://launchpad.net/gcc-arm-embedded/4.6/4.6-2012-q2-update/+download/$GCC_ARM_ZIP"
    GCC_ARM_DIR="gcc-arm-embedded"

    pushd $DEPENDENCIES_FOLDER
    if ! test -e $GCC_ARM_ZIP
    then
        wget $GCC_ARM_URL
    fi

    mkdir -p $GCC_ARM_DIR
    pushd $GCC_ARM_DIR

    GCC_INNER_DIR="gcc-arm-none-eabi-4_6-2012q2"
    if ! test -d $GCC_INNER_DIR
    then
        tar -xjf ../$GCC_ARM_ZIP
    fi

    if ! test -d arm-none-eabi
    then
        cp -R $GCC_INNER_DIR/* .
    fi

    popd
    popd

fi

if test ! $(which openocd); then

    ## Download OpenOCD for flashing ARM via JTAG

    if [ $OS == "linux" ]; then
        DISTRO=`lsb_release -si`

        if [ $DISTRO == "arch" ]; then
            sudo pacman -S openocd
        elif [ $DISTROj == "Ubuntu" ]; then
            sudo apt-get install openocd
        fi
    elif [ $OS == "osx" ]; then
        OPENOCD_BASENAME="openocd-0.6.1"
        OPENOCD_FILE="$OPENOCD_BASENAME.tar.bz2"
        OPENOCD_DOWNLOAD_URL="http://downloads.sourceforge.net/project/openocd/openocd/0.6.1/$OPENOCD_FILE"

        # look for homebrew
        brew install libftdi libusb
        wget $OPENOCD_DOWNLOAD_URL
        tar -xjf $OPENOCD_FILE
        pushd $OPENOCD_BASENAME

        ./configure --enable-ft2232_libftdi
        make
        sudo make install

        # TODO modify ftdi kernel module
        popd
    fi

fi
