#!/bin/bash

BIN=bin/fred-debug
#BIN=bin/fred-verbose

DOC=samples/raven.txt

for I in afl-out/crashes/id*; do
	echo $I
	$BIN -n $DOC < $I > /dev/null
	read
done

exit 0
