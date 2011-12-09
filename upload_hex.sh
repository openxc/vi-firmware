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
    PORT=`ls /dev/ttyUSB* | head -n 1`
fi

MCU=32MX795F512L # chipKIT Max32
AVRDUDE_ARD_PROGRAMMER=stk500v2
AVRDUDE_ARD_BAUDRATE=115200

AVRDUDE_TOOLS_PATH=$ARDUINO_HOME/hardware/tools
AVRDUDE=$AVRDUDE_TOOLS_PATH/avrdude
AVRDUDE_CONF=$AVRDUDE_TOOLS_PATH/avrdude.conf
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
