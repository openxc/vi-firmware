#!/bin/bash

MODEL=$1

if [ -z "$MODEL" ]
then
    echo "Swap to which vehicle model program? "
    read MODEL
fi

HANDLERS=../../cansignals/$MODEL/handlers.cpp
SIGNALS=../../cansignals/build/$MODEL/all.cpp

pushd cantranslator
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

ln -fs ../../cansignals/shared/handlers.cpp shared_handlers.cpp
ln -fs ../../cansignals/$MODEL/handlers.cpp
ln -fs ../../cansignals/build/$MODEL/all.cpp signals.cpp

echo "Swapped to $MODEL."
popd
