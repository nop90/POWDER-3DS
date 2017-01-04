#!/bin/bash

if [ -z "$CXXFLAGS" ]; then
    export CXXFLAGS=-O3
fi

PREFIX_DOC=$DESTDIR/usr/share/doc/powder
PREFIX_BIN=$DESTDIR/opt/bin

function usage {
    echo Usage: `basename $0` [options]
    echo -e "--install\t - install POWDER"
    echo -e "--uninstall\t - uninstall POWDER"
    echo -e "--help (-h)\t - this help"
    echo -e "--use-home-dir \t - compile with ~/.powder for saving data"
    echo 
    echo Script uses these environment variables:
    echo CXXFLAGS, DESTDIR
    echo Installation will be done to
    echo DESTDIR/usr/share/doc/powder
    echo DESTDIR/opt/bin
    echo If you do not specify anything, POWDER
    echo will just be compiled.
    echo Only one option can be present.
    echo -e "--install implies changing save to ~/.powder"
    echo -e "--install does *NOT* rebuild if powder exists"
}

function compile {
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
    cd ../../port/linux
    make clean
    echo Premake
    make premake
    echo Final install
    make powder-static
    cp powder ../..
    cd ../..
    echo Run powder to play
}

function install {
    if [ ! -f powder ]; then
	compile
    fi
    cp powder $PREFIX_BIN || exit 1
    mkdir -p $PREFIX_DOC || exit 1
    for file in README.TXT LICENSE.TXT CREDITS.TXT; do
	cp $file $PREFIX_DOC || exit 1
	done
    echo "Installation complete"
}

function uninstall {
    rm $PREFIX_BIN/powder
    for file in README.TXT LICENSE.TXT CREDITS.TXT; do
	rm $PREFIX_DOC/$file
    done
    rmdir $PREFIX_DOC
}

for arg in "$@"
do
    if [ "$arg" = "-h" ] || [ "$arg" = "--help" ]; then
	usage
	exit
    fi
    if [ "$arg" = "--install" ]; then
	CXXFLAGS="$CXXFLAGS -DCHANGE_WORK_DIRECTORY"
	INSTALL=1
    fi
    if [ "$arg" = "--use-home-dir" ]; then
	CXXFLAGS="$CXXFLAGS -DCHANGE_WORK_DIRECTORY"
    fi
    if [ "$arg" = "--uninstall" ]; then
	uninstall
	exit
    fi
done

if [ "$INSTALL" == "1" ]; then
    install
else
    compile
fi
