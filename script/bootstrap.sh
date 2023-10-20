#!/usr/bin/env bash

set -e

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd $SCRIPTS_DIR/..

if [ -z $CI ] && [ -z $VAGRANT ]; then
    echo "Which component do you wish to bootstrap?"
    select yn in "ARM Compiling" "PIC32 Compiling" "chipKIT Flashing" "Everything"; do
        case $yn in
            "ARM Compiling" ) source $SCRIPTS_DIR/bootstrap/arm.sh; break;;
            "PIC32 Compiling" ) source $SCRIPTS_DIR/bootstrap/pic32.sh; break;;
            "chipKIT Flashing" ) source $SCRIPTS_DIR/bootstrap/flashing_chipkit.sh; break;;
            "Firmware Development" ) source $SCRIPTS_DIR/bootstrap/devel.sh; break;;
            Everything )
                source $SCRIPTS_DIR/bootstrap/arm.sh
                # pic32.sh includes flashing_chipkit.sh
                source $SCRIPTS_DIR/bootstrap/pic32.sh
                source $SCRIPTS_DIR/bootstrap/devel.sh;
                break;;
        esac
    done
else
    # Everything - pic32.sh includes flashing_chipkit.sh
    source $SCRIPTS_DIR/bootstrap/arm.sh
    source $SCRIPTS_DIR/bootstrap/pic32.sh
    source $SCRIPTS_DIR/bootstrap/devel.sh;
fi
rm -rf ./dependencies
echo "Bootstrap complete"
