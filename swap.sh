#!/usr/bin/env bash

MODEL=$1
YEAR=$2

if [ -z "$MODEL" ]
then
    echo "Swap to which vehicle model program? "
    read MODEL
fi

if [ -z "$YEAR" ]
then
    echo "Swap to which model year? "
    read YEAR
fi

FULL_MODEL="$MODEL-$YEAR"

HANDLERS=../../cansignals/$FULL_MODEL/handlers.cpp
SIGNALS=../../cansignals/build/$FULL_MODEL.cpp

pushd cantranslator > /dev/null
if [ ! -f $HANDLERS ]
then
    echo "Couldn't find the handlers file at $HANDLERS"
    echo "Did you run \"make\" in cansignals?"
    exit 1
fi

if [ ! -f $SIGNALS ]
then
    echo "Couldn't find the signals file at $SIGNALS"
    echo "Did you run \"make\" in cansignals?"
    exit 1
fi

KERNEL=`uname -o`

if [ "Cygwin" == $KERNEL ]
then
    # Normal UNIX-style symlinks can't be read in Windows, and if we try to use
    # Windows-style links, MPIDE just ignores all of those files. We have to
    # explicitly copy them to the directory.
    cp -f ../../cansignals/shared/handlers.cpp shared_handlers.cpp
    cp -f ../../cansignals/$FULL_MODEL/handlers.cpp handlers.cpp
    cp -f ../../cansignals/build/$FULL_MODEL.cpp signals.cpp
else
    ln -fs ../../cansignals/shared/handlers.cpp shared_handlers.cpp
    ln -fs ../../cansignals/$FULL_MODEL/handlers.cpp
    ln -fs ../../cansignals/build/$FULL_MODEL.cpp signals.cpp
fi

echo "Swapped to $FULL_MODEL."
popd > /dev/null
