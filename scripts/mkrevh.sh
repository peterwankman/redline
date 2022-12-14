#!/bin/sh

REVISION=HEAD
REV=$(git rev-list --count $REVISION)

echo '/* THIS FILE HAS BEEN AUTOMATICALLY GENERATED */'
echo '#ifndef APP_VER_REV'
echo '#define APP_VER_REV' $REV
echo '#endif'

exit 0
