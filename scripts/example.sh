#!/bin/bash

BINARY=bin/fred
EDSCRIPT=scripts/unfunny.edl
SAMPLE=samples/lowerbible.txt
RESULT=raptorbible.txt

if [ ! -e $BINARY ]; then
	make
	mv bin/led bin/fred
fi

if [ ! -e $BINARY ]; then
	echo Can\'t find "$BINARY". Can\'t make it either.
fi

$BINARY $SAMPLE < $EDSCRIPT

echo Result is now in $RESULT
