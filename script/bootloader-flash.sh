#!/bin/bash -x

DEVICE=$1
FIRMWARE=$2

sudo mdel -i $DEVICE ::/firmware.bin
while [ $? != 0 ]; do
    sleep 2
    sudo mdel -i $DEVICE ::/firmware.bin
done

sudo mcopy -i $DEVICE $FIRMWARE ::/firmware.bin
