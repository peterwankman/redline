#!/bin/sh

afl-fuzz -i afl-in -o afl-out bin/edison-afl raven.txt
