#!/bin/sh

echo Building Universal Binary

echo Buidling both Version
rm -f powder
make clean
make

echo All done: result in powder.
