#!/usr/bin/env bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd $DIR/..

source $DIR/bootstrap_for_flashing.sh

CYGWIN_PACKAGES="make, gcc4, patchutils, git, unzip, python, check, curl, libsasl2, ca-certificates, python-setuptools"

if [ $OS == "windows" ]; then
    die "Sorry, the bootstrap script for compiling from source doesn't support the Windows console - try Cygwin."
fi

if [ $OS == "mac" ] && ! command -v brew >/dev/null 2>&1; then
    echo "Installing Homebrew..."
    ruby -e "$(curl -fsSkL raw.github.com/mxcl/homebrew/go)"
fi

if ! command -v make >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "make"
    elif [ $OS == "mac" ]; then
            die "Missing 'make' - install the Xcode CLI tools"
    else
        if [ $DISTRO == "arch" ]; then
            sudo pacman -S base-devel
        elif [ $DISTRO == "Ubuntu" ]; then
            sudo apt-get update -qq
            sudo apt-get install build-essential -y
        fi
    fi
fi

if ! command -v git >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "git"
    elif [ $OS == "mac" ]; then
        _install git
    fi
fi

echo "Updating Git submodules..."

# git submodule update is a shell script and expects some lines to fail
set +e
if ! git submodule update --init --quiet; then
    echo "Unable to update git submodules - try running \"git submodule update\" to see the full error"
    echo "If git complains that it \"Needed a single revision\", run \"rm -rf src/libs\" and then try the bootstrap script again"
    if [ $OS == "cygwin" ]; then
        echo "In Cygwin this may be true (ignore if you know ca-certifications is installed:"
        _cygwin_error "ca-certificates"
    fi
    die
fi
set -e

echo "Installing dependencies for running test suite..."

if [ -z $CI ] && ! command -v lcov >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        echo "Missing lcov - Cygwin doesn't have a packaged version of lcov, and it's only required to calculate test suite coverage. We'll skip it."
    elif [ $OS == "mac" ]; then
        brew install lcov
    else
        if [ $DISTRO == "arch" ]; then
            echo "Missing lcov - install from the AUR."
            _wait
        elif [ $DISTRO == "Ubuntu" ]; then
            sudo apt-get update -qq
            sudo apt-get install lcov -y
        fi
    fi
fi

if [ $OS == "mac" ]; then
    _pushd $DEPENDENCIES_FOLDER
    LLVM_BASENAME=clang+llvm-3.2-x86_64-apple-darwin11
    LLVM_FILE=$LLVM_BASENAME.tar.gz
    LLVM_URL=http://llvm.org/releases/3.2/$LLVM_FILE

    if ! test -e $LLVM_FILE
    then
        echo "Downloading LLVM 3.2..."
        download $LLVM_URL $LLVM_FILE
    fi

    if ! test -d $LLVM_BASENAME
    then
        echo "Installing LLVM 3.2 to local folder..."
        tar -xzf $LLVM_FILE
        echo "LLVM 3.2 installed"
    fi

    _popd
fi

echo "Installing dependencies for building for chipKIT Max32 platform"

## chipKIT libraries for USB, CAN and Network

CHIPKIT_LIBRARY_AGREEMENT_URL="http://www.digilentinc.com/Agreement.cfm?DocID=DSD-0000318"
CHIPKIT_LIBRARY_DOWNLOAD_URL="http://www.digilentinc.com/Data/Documents/Product%20Documentation/chipKIT%20Network%20and%20USB%20Libs.zip"
CHIPKIT_ZIP_FILE="chipkit.zip"

_pushd $DEPENDENCIES_FOLDER
if ! test -e chipkit.zip
then
    echo
    if [ -z $CI ]; then
        echo "By running this command, you agree to Microchip's licensing agreement at $CHIPKIT_LIBRARY_AGREEMENT_URL"
        echo "Press Enter to verify you have read the license agreement."
        read
    fi
    download $CHIPKIT_LIBRARY_DOWNLOAD_URL $CHIPKIT_ZIP_FILE
fi
_popd

_pushd src/libs
for LIBRARY in chipKITUSBDevice chipKITCAN chipKITEthernet; do
    if ! test -d $LIBRARY
    then
        echo "Installing chipKIT library $LIBRARY..."
        unzip ../../dependencies/$CHIPKIT_ZIP_FILE "$LIBRARY/*"
    fi
done
_popd

### Patch libraries to avoid problems in case sensitive operating systems
### See https://github.com/chipKIT32/chipKIT32-MAX/issues/146
### and https://github.com/chipKIT32/chipKIT32-MAX/issues/199

echo "Patching case-sensitivity bugs in chipKIT libraries..."

if [ $OS == "cygwin" ] && ! [ -e /usr/bin/patch ]; then
    _cygwin_error "patchutils"
fi

# If the patch is already applied, patch will error out, so disable quit on
# error temporarily
set +e
_pushd src/libs
_pushd chipKITUSBDevice
patch -p1 -sNi ../../../script/chipKITUSBDevice-case.patch > /dev/null
_popd

_pushd chipKITCAN
patch -p1 -sNi ../../../script/chipKITCAN-case.patch > /dev/null
_popd

_popd
set -e

# ARM / LPC17XX Dependencies

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then

    echo "Installing GCC for ARM Embedded..."

    GCC_ARM_BASENAME="gcc-arm-none-eabi-4_7-2012q4-20121208"
    if [ $OS == "linux" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-linux.tar.bz2"
    elif [ $OS == "mac" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-mac.tar.bz2"
    elif [ $OS == "cygwin" ]; then
        GCC_ARM_FILE="$GCC_ARM_BASENAME-win32.exe"
    fi

    GCC_ARM_URL="https://launchpad.net/gcc-arm-embedded/4.7/4.7-2012-q4-major/+download/$GCC_ARM_FILE"
    GCC_ARM_DIR="gcc-arm-embedded"

    _pushd $DEPENDENCIES_FOLDER
    if ! test -e $GCC_ARM_FILE
    then
        download $GCC_ARM_URL $GCC_ARM_FILE
    fi

    mkdir -p $GCC_ARM_DIR
    _pushd $GCC_ARM_DIR
    if [ $OS == "cygwin" ]; then
        chmod a+x ../$GCC_ARM_FILE
        INSTALL_COMMAND="cygstart.exe ../$GCC_ARM_FILE"
        PROGRAM_FILES_BASE="/cygdrive/c/"
        PROGRAM_FILES="Program Files"
        PROGRAM_FILES_64="Program Files (x86)"
        TRAILING_DIRNAME="GNU Tools ARM Embedded/4.7 2012q4/"
        GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES_64/$TRAILING_DIRNAME"
        if ! test -d "$GCC_INNER_DIR"; then
            GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES/$TRAILING_DIRNAME"
        fi
    else
        GCC_INNER_DIR="gcc-arm-none-eabi-4_7-2012q4"
        INSTALL_COMMAND="tar -xjf ../$GCC_ARM_FILE"
    fi

    if ! test -d "$GCC_INNER_DIR"
    then
        $INSTALL_COMMAND
        if [ $OS == "cygwin" ]; then
            echo -n "Press Enter when the GCC for ARM Embedded installer is finished"
            read
        fi
    fi

    if [ $OS == "cygwin" ]; then
        GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES_64/$TRAILING_DIRNAME"
        if ! test -d "$GCC_INNER_DIR"; then
            GCC_INNER_DIR="$PROGRAM_FILES_BASE/$PROGRAM_FILES/$TRAILING_DIRNAME"
            if ! test -d "$GCC_INNER_DIR"; then
                die "GCC for ARM isn't installed in the expected location."
            fi
        fi
    fi

    if ! test -d arm-none-eabi; then
        echo "Copying GCC binaries to local dependencies folder..."
        cp -R "$GCC_INNER_DIR"/* .
    fi

    _popd
    _popd

fi

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
if [ -z $CI ]  && [ $OS == "mac" ] && [ -e $FTDI_USB_DRIVER_PLIST ]; then
    if grep -q "Olimex OpenOCD JTAG A" $FTDI_USB_DRIVER_PLIST; then
        sudo sed -i "" -e "/Olimex OpenOCD JTAG A/{N;N;N;N;N;N;N;N;N;N;N;N;N;N;N;N;d;}" $FTDI_USB_DRIVER_PLIST
        FTDI_USB_DRIVER_MODULE=/System/Library/Extensions/FTDIUSBSerialDriver.kext/
        # Driver may not be loaded yet, but that's OK - don't exit on error.
        set +e
        sudo kextunload $FTDI_USB_DRIVER_MODULE
        set -e
        sudo kextload $FTDI_USB_DRIVER_MODULE
    fi
fi

if [ $OS == "cygwin" ] && ! command -v ld >/dev/null 2>&1; then
    _cygwin_error "gcc4"
fi

if ! ld -lcheck -o /tmp/checkcheck 2>/dev/null; then
    echo "Installing the check unit testing library..."

    _install "check"
fi

if ! command -v python >/dev/null 2>&1; then
    echo "Installing Python..."
    _install "python"
fi

if ! python -c "import argparse"; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "python-argparse"
    fi
fi

# TODO this is kind of a hacky way of determining if root is required -
# ideally we wouuld set up a little virtualenv in the dependencies folder
SUDO_CMD=
if command -v sudo >/dev/null 2>&1; then
    SUDO_CMD=sudo
fi

if ! command -v pip >/dev/null 2>&1; then
    echo "Installing Python..."
    if ! command -v easy_install >/dev/null 2>&1; then
        die "easy_install not available, can't install pip"
    fi

    $SUDO_CMD easy_install pip
fi

if ! python -c "import openxc" || ! command -v openxc-generate-firmware-code >/dev/null 2>&1; then
    $SUDO_CMD pip install -U openxc
fi

popd

echo
echo "${bldgreen}All developer dependencies installed, ready to compile.$txtrst"
