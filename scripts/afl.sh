#!/bin/bash

AFL_IN=afl-in/
AFL_OUT=afl-out/
JEDI_BIN=bin/afl-ed
DOC=lowerbible.txt

if [ -z "$2" ]; then
	AFL_BIN=afl-fuzz
else
	AFL_BIN="$2"
fi

"$AFL_BIN" -m none -i "$AFL_IN" -o "$AFL_OUT" -- "$JEDI_BIN" "$DOC"
