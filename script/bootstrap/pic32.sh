set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

echo "Installing dependencies for building for chipKIT Max32 platform"

source $BOOTSTRAP_DIR/flashing_chipkit.sh

## chipKIT libraries for USB, CAN and Network

CHIPKIT_LIBRARY_AGREEMENT_URL="https://reference.digilentinc.com/agreement"

# Disabling SSL cert checking (-k), which while strong discouraged, is
# used here because some dependency hosts CA bundle files are messed up,
# and this software doesn't store any secure data. If Digilent fixes
# their SSL certificate bundle we can remove it.
NOT_SECURE="true"
CHIPKIT_LIBRARY_DOWNLOAD_URL="https://reference.digilentinc.com/_media/chipkit_network_and_usb_libs-20150115.zip"
CHIPKIT_ZIP_FILE="chipkit_network_and_usb_libs-20150115.zip"

_pushd $DEPENDENCIES_FOLDER
if ! test -e $CHIPKIT_ZIP_FILE
then
    echo
    if [ -z $CI ] && [ -z $VAGRANT ]; then
        echo "By running this command, you agree to Microchip's licensing agreement at $CHIPKIT_LIBRARY_AGREEMENT_URL"
        echo "Press Enter to verify you have read the license agreement."
        read
    fi
    download $CHIPKIT_LIBRARY_DOWNLOAD_URL $CHIPKIT_ZIP_FILE $NOT_SECURE
    echo "Extracting CHIPKit"
    unzip -q $CHIPKIT_ZIP_FILE
    rm -rf $CHIPKIT_ZIP_FILE
fi
_popd

_pushd src/libs
for LIBRARY in chipKITUSBDevice chipKITCAN chipKITEthernet; do
    echo "Installing chipKIT library $LIBRARY..."
    sudo cp -R ../../dependencies/libraries/$LIBRARY /usr/lib/chipkit
done
rm -rf ../../dependencies/libraries
_popd

### Patch libraries to avoid problems in case sensitive operating systems
### See https://github.com/chipKIT32/chipKIT32-MAX/issues/146
### and https://github.com/chipKIT32/chipKIT32-MAX/issues/199

echo "Patching case-sensitivity bugs in chipKIT libraries..."

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

#$PIP_SUDO_CMD pip install --upgrade pyserial

echo
echo "${bldgreen}PIC32 / chipKIT compilation dependencies installed.$txtrst"
