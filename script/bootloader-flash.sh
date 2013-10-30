#!/bin/bash
#
# Automate re-flashing an LPC17xx device running the USB bootloader from Linux.
#
# Example:
#    $ ./flash.sh /dev/sdc new-firmware.bin
#

DEVICE=$1
FIRMWARE=$2

echo "Deleting existing firmware..."
sudo mdel -i $DEVICE ::/firmware.bin
while [ $? != 0 ]; do
    sleep 2

    echo "Re-trying delete of existing firwmare..."
    sudo mdel -i $DEVICE ::/firmware.bin
done

echo "Copying new firmware..."
sudo mcopy -i $DEVICE $FIRMWARE ::/firmware.bin
echo "Done."
