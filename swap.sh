#!/usr/bin/env bash

MODEL=$1

if [ -z "$MODEL" ]
then
    echo "Swap to which vehicle model program? "
    read MODEL
fi

ln -fs ../../cansignals/shared/handlers.cpp cantranslator/shared_handlers.cpp
ln -fs ../../cansignals/$MODEL/handlers.cpp cantranslator
ln -fs ../../cansignals/build/$MODEL/all.cpp cantranslator/signals.cpp

echo "Swapped to $MODEL."
