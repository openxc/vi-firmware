set -e
BOOTSTRAP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BOOTSTRAP_DIR/common.sh

echo "Installing dependencies for building for chipKIT Max32 platform"

source $BOOTSTRAP_DIR/flashing_chipkit.sh

if !  command -v p7zip >/dev/null 2>&1; then
	echo "Installing 7zip..."
	_install "p7zip-full"
fi

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

MLA_LIBRARY_AGREEMENT_URL="http://ww1.microchip.com/downloads/en/DeviceDoc/Microchip%20Application%20Solutions%20Users%20Agreement.pdf"
MLA_LIBRARY_URL="http://ww1.microchip.com/downloads/en/DeviceDoc/MCHP_App_Lib_v2010_10_19_Installer.zip"
MLA_ZIP_FILE="MCHP_App_Lib_v2010_10_19_Installer.zip"
MLA_FOLDER_OUTPUT="MLA"
if ! test -e $MLA_ZIP_FILE 
then
    echo
    if [ -z $CI ] && [ -z $VAGRANT ]; then
        echo "By running this command, you agree to Microchip's licensing agreement at $MLA_LIBRARY_AGREEMENT_URL"
        echo "Press Enter to verify you have read the license agreement."
        read
    fi
    download $MLA_LIBRARY_URL $MLA_ZIP_FILE
    unzip $MLA_ZIP_FILE
	echo "Extracting MLA"
	7z x 'Microchip Application Libraries v2010-10-19 Installer.exe' -o$MLA_FOLDER_OUTPUT
fi
_popd


_pushd src/libs
for LIBRARY in chipKITUSBDevice chipKITCAN chipKITEthernet; do
    echo "Installing chipKIT library $LIBRARY..."
    cp -R ../../dependencies/libraries/$LIBRARY .
done

if ! test -e $MLA_FOLDER_OUTPUT
then
echo "Installing MLA MSD"
mkdir $MLA_FOLDER_OUTPUT
mkdir $MLA_FOLDER_OUTPUT/MDD_File_System
cp -R '../../dependencies/MLA/Microchip/USB/MSD Device Driver/' ./MLA/MSD_Device_Driver
cp -R '../../dependencies/MLA/Microchip/MDD File System/' ./MLA/MDD_File_System
cp -R '../../dependencies/MLA/Microchip/Include/' ./MLA/Include
fi

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

echo "Patching FSIO.C file to add flushing operation"
set +e
_pushd src/libs/MLA/MDD_File_System
patch -p1 -sNi ../../../../script/FSIO-flush.patch > /dev/null
patch -p1 -sNi ../../../../script/SD-SPI-platform.patch > /dev/null
_popd
set -e


## Python pyserial module for the reset script in Arduino-Makefile

$PIP_SUDO_CMD pip install --upgrade pyserial

echo
echo "${bldgreen}PIC32 / chipKIT compilation dependencies installed.$txtrst"
