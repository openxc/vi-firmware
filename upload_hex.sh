#!/bin/sh
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
HEX_FILE=$1
PORT=$2

die() {
    echo >&2 "$@"
    exit 1
}

if [ -z $PORT ]; then
    if [ ${KERNEL:0:7} == "MINGW32" ]; then
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

if [ -z $MPIDE_DIR ]; then
    echo "The environment variable MPIDE_DIR doesn't point to an MPIDE \
installation."
    if [ -z $AVRDUDE ]; then
        AVRDUDE=`which avrdude`
    fi

    if [ -z $AVRDUDE_CONF ]; then
        AVRDUDE_CONF="$DIR/conf/avrdude.conf"
    fi

    if [ -z $AVRDUDE ]; then
        die "ERROR: No avrdude binary found in your path"
    else
        echo "Using $AVRDUDE with config $AVRDUDE_CONF..."
    fi
else
    echo "Using avrdude from MPIDE installation..."
    AVRDUDE_TOOLS_PATH=$MPIDE_DIR/hardware/tools
    if [ "`uname`" = "Darwin" ]; then
        AVRDUDE=$AVRDUDE_TOOLS_PATH/avr/bin/avrdude
        AVRDUDE_CONF=$AVRDUDE_TOOLS_PATH/avr/etc/avrdude.conf
    else
        AVRDUDE=$AVRDUDE_TOOLS_PATH/avrdude
        AVRDUDE_CONF=$AVRDUDE_TOOLS_PATH/avrdude.conf
    fi

fi

MCU=32MX795F512L # chipKIT Max32
AVRDUDE_ARD_PROGRAMMER=stk500v2
AVRDUDE_ARD_BAUDRATE=115200

AVRDUDE_COM_OPTS="-q -V -p $MCU -C $AVRDUDE_CONF"
AVRDUDE_ARD_OPTS="-c $AVRDUDE_ARD_PROGRAMMER -b $AVRDUDE_ARD_BAUDRATE -P $PORT"

[ "$#" -eq 1 ] || die "path to hex file is required as a parameter"

upload() {
    $AVRDUDE $AVRDUDE_COM_OPTS $AVRDUDE_ARD_OPTS -U flash:w:$HEX_FILE:i
}

reset() {
    for STTYF in 'stty --file' 'stty -f' 'stty <' ; \
      do $STTYF /dev/tty >/dev/null 2>/dev/null && break ; \
    done ;\
    $STTYF $PORT  hupcl ;\
    (sleep 0.1 || sleep 1)     ;\
    $STTYF $PORT -hupcl
}

if [ ${KERNEL:0:6} != "MINGW32" ]; then
    # no stty in windows, so we just skip it - you need to run this script right
    # after you plug in the board so it's still in programmable mode.
    reset
fi

upload
