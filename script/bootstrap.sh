#!/usr/bin/env bash

set -e

die() {
    echo >&2 "$@"
    exit 1
}

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

_cygwin_error() {
    echo
    echo "Missing \"$1\" - run the Cygwin installer again and select the base package set:"
    echo "    gcc4, patchutils, git, unzip, python, python-argparse, check, curl, libsasl2, ca-certificates"
    die
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
    DISTRO=`lsb_release -si`
fi

download() {
    url=$1
    filename=$2
    curl $url -L --O $filename
}

if [ $OS == "cygwin" ] && ! command -v curl >/dev/null 2>&1; then
    _cygwin_error "curl"
fi

if ! command -v make >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "make"
    else
        if [ $DISTRO == "arch" ]; then
            sudo pacman -S base-devel
        elif [ $DISTRO == "Ubuntu" ]; then
            sudo apt-get update -qq
            sudo apt-get install build-essential
        fi
    fi
fi

if ! command -v git >/dev/null 2>&1; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "git"
    elif [ $OS == "mac" ]; then
        brew install git
    else
        if [ $DISTRO == "arch" ]; then
            sudo pacman -S git
        elif [ $DISTRO == "Ubuntu" ]; then
            sudo apt-get update -qq
            sudo apt-get install git
        fi
    fi
fi

if [ $OS != "cygwin" ] && ! command -v lcov >/dev/null 2>&1; then
    echo "Missing lcov - Cygwin doesn't have a packaged version of lcov, and it's only required to calculate test suite coverage. We'll skip it."
    if [ $OS == "mac" ]; then
        brew install lcov
    else
        if [ $DISTRO == "arch" ]; then
            echo "Missing lcov - install from the AUR."
            _wait
        elif [ $DISTRO == "Ubuntu" ]; then
            sudo apt-get update -qq
            sudo apt-get install lcov
        fi
    fi
fi

echo "Updating Git submodules..."

git submodule update --init --quiet

echo "Storing all downloaded dependencies in the \"dependencies\" folder"

DEPENDENCIES_FOLDER="dependencies"
mkdir -p $DEPENDENCIES_FOLDER

echo "Installing dependencies for building for chipKIT Max32 platform"

if [ -z "$MPIDE_DIR" ] || ! test -e $MPIDE_DIR; then

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
        echo "Downloaded MPIDE..."
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

## chipKIT libraries for USB, CAN and Ethernet

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
        if [ $DISTRO == "arch" ]; then
            sudo pacman -S openocd
        elif [ $DISTRO == "Ubuntu" ]; then
            sudo apt-get update -qq
            sudo apt-get install openocd
        else
            echo "Missing OpenOCD - install it using your distro's package manager or build from source"
            _wait
        fi
    elif [ $OS == "osx" ]; then
        OPENOCD_BASENAME="openocd-0.6.1"
        OPENOCD_FILE="$OPENOCD_BASENAME.tar.bz2"
        OPENOCD_DOWNLOAD_URL="http://downloads.sourceforge.net/project/openocd/openocd/0.6.1/$OPENOCD_FILE"

        # look for homebrew
        brew install libftdi libusb
        download $OPENOCD_DOWNLOAD_URL $OPENOCD_FILE
        tar -xjf $OPENOCD_FILE
        _pushd $OPENOCD_BASENAME

        ./configure --enable-ft2232_libftdi
        make
        sudo make install

        # TODO modify ftdi kernel module
        _popd
    elif [ $OS == "cygwin" ]; then
        echo
        echo "Missing OpenOCD and it's not trivial to install in Windows - you won't be able to program the ARM platform"
    fi
    _popd

fi

if [ $OS == "cygwin" ] && ! command -v ld >/dev/null 2>&1; then
    _cygwin_error "ld"
fi

if ! ld -lcheck -o /tmp/checkcheck 2>/dev/null; then

    echo "Installing the check unit testing library..."

    if [ $OS == "cygwin" ]; then
        _cygwin_error "check"
    elif [ $OS == "linux" ]; then
        if ! command -v lsb_release >/dev/null 2>&1; then
            echo
            echo "Missing the 'check' library - install it using your distro's package manager or build from source"
        else
            if [ $DISTRO == "arch" ]; then
                sudo pacman -S check
            elif [ $DISTRO == "Ubuntu" ]; then
                sudo apt-get update -qq
                sudo apt-get install check
            else
                echo
                echo "Missing the 'check' library - install it using your distro's package manager or build from source"
                _wait
            fi
        fi
    elif [ $OS == "osx" ]; then
        # brew exists with 1 if it's already installed
        set +e
        brew install check
        set -e
    fi
fi

if ! command -v python >/dev/null 2>&1; then
    echo "Installing Python..."
    if [ $OS == "cygwin" ]; then
        _cygwin_error "python"
    elif [ $OS == "linux" ]; then
        if [ $DISTRO == "arch" ]; then
            sudo pacman -S python
        elif [ $DISTRO == "Ubuntu" ]; then
            sudo apt-get install python
        else
            echo "Missing Python - install it using your distro's package manager or build from source"
            _wait
        fi
     fi
fi

if ! python -c "import argparse"; then
    if [ $OS == "cygwin" ]; then
        _cygwin_error "python-argparse"
    fi
fi

echo
echo "All mandatory dependencies installed, ready to compile."
