#!/usr/bin/env bash

set -e

CHIPKIT_LIBRARY_AGREEMENT_URL="http://www.digilentinc.com/Agreement.cfm?DocID=DSD-0000318"
CHIPKIT_LIBRARY_DOWNLOAD_URL="http://www.digilentinc.com/Data/Documents/Product%20Documentation/chipKIT%20Network%20and%20USB%20Libs.zip"

git submodule update --init

if ! test -e mpide-0023-linux-20120903.tgz
then
    wget https://github.com/downloads/chipKIT32/chipKIT32-MAX/mpide-0023-linux-20120903.tgz
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
