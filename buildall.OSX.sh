#!/bin/sh

echo Run this from the root of the source tree.

echo Rebuild all of POWDER
echo Build support binaries
cd support/bmp2c
make clean
make
cd ../encyclopedia2c
make clean
make
cd ../enummaker
make clean
make
cd ../map2c
make clean
make
cd ../txt2c
make clean
make
cd ../tile2c
make clean
make

echo Clean...
cd ../../port/mac
make clean
echo Premake
make premake
echo Final install
./buildubx.sh
cp powder ../..
cd ../..
echo Run powder to play
