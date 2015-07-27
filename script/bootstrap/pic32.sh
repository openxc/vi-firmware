set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

echo "Installing dependencies for building for chipKIT Max32 platform"

source $BOOTSTRAP_DIR/flashing_chipkit.sh

## chipKIT libraries for USB, CAN and Network

CHIPKIT_LIBRARY_AGREEMENT_URL="http://www.digilentinc.com/Agreement.cfm?DocID=DSD-0000318"
CHIPKIT_LIBRARY_DOWNLOAD_URL="http://www.digilentinc.com/Data/Documents/Product%20Documentation/chipKIT%20Network%20and%20USB%20Libs-20130724a.zip"
CHIPKIT_ZIP_FILE="chipkit-libraries-2013-07-24.zip"

_pushd $DEPENDENCIES_FOLDER
if ! test -e $CHIPKIT_ZIP_FILE
then
    echo
    if [ -z $CI ] && [ -z $VAGRANT ]; then
        echo "By running this command, you agree to Microchip's licensing agreement at $CHIPKIT_LIBRARY_AGREEMENT_URL"
        echo "Press Enter to verify you have read the license agreement."
        read
    fi
    download $CHIPKIT_LIBRARY_DOWNLOAD_URL $CHIPKIT_ZIP_FILE
    unzip $CHIPKIT_ZIP_FILE
fi
_popd

_pushd src/libs
for LIBRARY in chipKITUSBDevice chipKITCAN chipKITEthernet; do
    echo "Installing chipKIT library $LIBRARY..."
    cp -R ../../dependencies/libraries/$LIBRARY .
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

## Python pyserial module for the reset script in Arduino-Makefile

pip install --user --upgrade pyserial

echo
echo "${bldgreen}PIC32 / chipKIT compilation dependencies installed.$txtrst"
