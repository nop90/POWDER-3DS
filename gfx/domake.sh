#!/bin/csh

echo Building Tiles...
foreach tileset ( classic distorted adambolt nethack ascii akoimeexx akoi12 akoi10 ibsongrey lomaka )
	pushd $tileset
	echo Building tiles...
	tile2c
	popd
end

pushd akoi3x
    echo Building Akoi3x
    gzip -d -c sprite16_3x.bmp.gz > sprite16_3x.bmp
    bmp2c sprite16_3x.bmp
popd

echo Building Background images..
bmp2c tridude_goodbye.bmp
bmp2c tridude_goodbye_hires.bmp
bmp2c icon_sdl.bmp

echo Dirtying build...
rm -f all_bitmaps.o
