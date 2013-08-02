#!/bin/bash
#
# Upload a PIC32 compatible application compiled to a .hex file to a device.
#
#    ./upload_hex.sh <path to hex file>
#
# This functionality is mostly copied from the Makefile so normal developers
# don't need to have that installed.
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
KERNEL=`uname`
HEX_FILE=`realpath --relative-to=$DIR/.. "$1"`
PORT=$2

source $DIR/bootstrap_for_flashing.sh

if [ -z $PORT ]; then
    if [ $OS == "windows" ] || [ $OS == "cygwin" ]; then
        PORT="com3"
    else
        PORT=`ls /dev/ttyUSB* 2> /dev/null | head -n 1`
        if [ -z $PORT ]; then
            PORT=`ls /dev/tty.usbserial* 2> /dev/null | head -n 1`
            if [ -z $PORT ]; then
                die "No CAN translator found - is it plugged in?"
            fi
        fi
    fi
fi

if [ -z "$MPIDE_DIR" ]; then
    MPIDE_DIR="$DIR/../$DEPENDENCIES_FOLDER/mpide"
fi

if ! [ -d "$MPIDE_DIR" ]; then
    echo "No MPIDE_DIR environment variable found, will use standalone avrdude"
    if [ -z "$AVRDUDE" ]; then
        AVRDUDE="`which avrdude`"
    fi

    if [ -z "$AVRDUDE" ]; then
        if [ $OS == "windows" ] || [ $OS == "cygwin" ]; then
            echo "Install WinAVR to get avrdude"
        fi
        die "ERROR: No avrdude binary found in your path"
    else
        echo "Using $AVRDUDE..."
    fi
else
    echo "Using avrdude from MPIDE installation at $MPIDE_DIR..."
    AVRDUDE_TOOLS_PATH="$MPIDE_DIR/hardware/tools"
    AVRDUDE="$AVRDUDE_TOOLS_PATH/avrdude"

    if [ ! -e "$AVRDUDE" ]; then
        # OS X and Windows MPIDE have different directory structure
        AVRDUDE="$AVRDUDE_TOOLS_PATH/avr/bin/avrdude"
    fi
fi

if [ -z $AVRDUDE_CONF ]; then
    if [ $OS == "cygwin" ]; then
        # avrdude in cygwin expects windows style paths, so the absolute
        # path throws a "file not found" error
        AVRDUDE_CONF="conf/avrdude.conf"
    else
        AVRDUDE_CONF="$DIR/../conf/avrdude.conf"
    fi
fi


MCU=32MX795F512L # chipKIT Max32
AVRDUDE_ARD_PROGRAMMER=stk500v2
AVRDUDE_ARD_BAUDRATE=115200

AVRDUDE_COM_OPTS="-q -V -p $MCU"
AVRDUDE_ARD_OPTS="-c $AVRDUDE_ARD_PROGRAMMER -b $AVRDUDE_ARD_BAUDRATE -P $PORT"

if [ -z "$HEX_FILE" ]; then
    die "path to hex file is required as a parameter, e.g.: script/upload_hex.sh <path to hex file>"
fi

upload() {
    "$AVRDUDE" -C "$AVRDUDE_CONF" $AVRDUDE_COM_OPTS $AVRDUDE_ARD_OPTS -U \
            flash:w:"$HEX_FILE":i
}

reset() {
    for STTYF in 'stty --file' 'stty -f' 'stty <' ; \
      do $STTYF /dev/tty >/dev/null 2>/dev/null && break ; \
    done ;\
    $STTYF $PORT  hupcl ;\
    (sleep 0.1 || sleep 1)     ;\
    $STTYF $PORT -hupcl
}

if [ $OS != "windows" ] && [ $OS != "cygwin" ]; then
    # no stty in windows and it doesn't work in cygwin, so we just skip it - you
    # need to run this script right after you plug in the board so it's still in
    # programmable mode.
    reset
fi

# avrdude in cygwin expects windows style paths, so we have to use a relative
# path to the avrdude.conf and must run from a predictable directory
pushd $DIR/..
pwd

echo "Flashing $HEX_FILE to device at port $PORT in $OS"
upload
echo "${bldgreen}Flashing completed successfully${txtrst}"

popd
