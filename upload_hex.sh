#!/bin/sh
#
# Upload a PIC32 compatible application compiled to a .hex file to a device.
#
#    ./upload_hex.sh <path to hex file>
#
# This functionality is mostly copied from the Makefile so normal developers
# don't need to have that installed.
#

HEX_FILE=$1
PORT=$2
if [ -z $PORT ]; then
    PORT=`ls /dev/ttyUSB* 2> /dev/null | head -n 1`
    if [ -z $PORT ]; then
        echo "No CAN translator found - is it plugged in?"
        exit 1
    fi
fi

if [ -z $ARDUINO_DIR ]; then
    echo "You must set the ARDUINO_DIR environment variable to the path \
to your MPIDE installation before using this command."
    exit 1
fi

MCU=32MX795F512L # chipKIT Max32
AVRDUDE_ARD_PROGRAMMER=stk500v2
AVRDUDE_ARD_BAUDRATE=115200

AVRDUDE_TOOLS_PATH=$ARDUINO_DIR/hardware/tools
if [ "`uname`" = "Darwin" ]; then
    AVRDUDE=$AVRDUDE_TOOLS_PATH/avr/bin/avrdude
    AVRDUDE_CONF=$AVRDUDE_TOOLS_PATH/avr/etc/avrdude.conf
else
    AVRDUDE=$AVRDUDE_TOOLS_PATH/avrdude
    AVRDUDE_CONF=$AVRDUDE_TOOLS_PATH/avrdude.conf
fi

AVRDUDE_COM_OPTS="-q -V -p $MCU -C $AVRDUDE_CONF"
AVRDUDE_ARD_OPTS="-c $AVRDUDE_ARD_PROGRAMMER -b $AVRDUDE_ARD_BAUDRATE -P $PORT"

die () {
    echo >&2 "$@"
    exit 1
}

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

reset
upload
