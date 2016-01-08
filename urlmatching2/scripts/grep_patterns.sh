#!/bin/bash

if [ -z "$1" ]
  then
    echo "Error: no source file"
	exit 1
fi

OUTPUT=$1
if [ -n "$2" ]
  then
	OUTPUT=$2
fi
SRC=$1

head -2 $SRC > $OUTPUT.tmp && tail -n +133 $SRC  >> $OUTPUT.tmp && mv $OUTPUT.tmp $OUTPUT 

