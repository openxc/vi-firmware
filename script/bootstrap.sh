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

CHIPKIT_LIBRARY_AGREEMENT_URL="http://www.digilentinc.com/Agreement.cfm?DocID=DSD-0000318"
CHIPKIT_LIBRARY_DOWNLOAD_URL="http://www.digilentinc.com/Data/Documents/Product%20Documentation/chipKIT%20Network%20and%20USB%20Libs.zip"

if [ $OS == "cgywin" ]; then
    MPIDE_URL=https://github.com/downloads/chipKIT32/chipKIT32-MAX/mpide-0023-windows-20120903.zip
elif [ $OS == "mac" ]; then
    # TODO how can we install this locally to the mpide directory? alternatively,
    # what if we just used the linux version in OS X?
    MPIDE_URL=https://github.com/downloads/chipKIT32/chipKIT32-MAX/mpide-0023-macosx-20120903.dmg
else
    MPIDE_URL=https://github.com/downloads/chipKIT32/chipKIT32-MAX/mpide-0023-linux-20120903.tgz
fi

git submodule update --init

if ! test -e mpide-0023-linux-20120903.tgz
then
    wget $MPIDE_URL
fi

if ! test -d mpide
then
    tar -xzf mpide-0023-linux-20120903.tgz
    mv mpide-0023-linux-20120903 mpide
fi

pushd src/libs
if ! test -e chipkit.zip
then
    echo "By running this command, you agree to Microchip's licensing agreement at $CHIPKIT_LIBRARY_AGREEMENT_URL"
    wget $CHIPKIT_LIBRARY_DOWNLOAD_URL -O chipkit.zip
fi

for LIBRARY in chipKITUSBDevice chipKITCAN chipKITEthernet; do
    if ! test -d $LIBRARY
    then
        unzip chipkit.zip "$LIBRARY/*"
    fi
done

# If the patch is already applied, patch will error out
set +e
pushd chipKITUSBDevice
patch -p1 -sNi ../../../script/chipKITUSBDevice-case.patch
popd

pushd chipKITCAN
patch -p1 -sNi ../../../script/chipKITCAN-case.patch
popd
set -e

popd

GCC_ARM_ZIP="gcc-arm-none-eabi-4_6-2012q2-20120614.tar.bz2"
GCC_ARM_URL="https://launchpad.net/gcc-arm-embedded/4.6/4.6-2012-q2-update/+download/$GCC_ARM_ZIP"
GCC_ARM_DIR="gcc-arm-embedded"
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
