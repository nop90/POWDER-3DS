@echo off
if !%1==! goto error

echo Prebuilding tiles...
pushd .
echo Building tileset classic
cd ..\..\..\gfx\classic
..\..\port\windows\%1\tile2c.exe
echo Building tileset distorted
cd ..\distorted
..\..\port\windows\%1\tile2c.exe
echo Building tileset adambolt
cd ..\adambolt
..\..\port\windows\%1\tile2c.exe
echo Building tileset nethack
cd ..\nethack
..\..\port\windows\%1\tile2c.exe
echo Building tileset ascii
cd ..\ascii
..\..\port\windows\%1\tile2c.exe
echo Building tileset akoimeexx
cd ..\akoimeexx
..\..\port\windows\%1\tile2c.exe
echo Building tileset akoi10
cd ..\akoi10
..\..\port\windows\%1\tile2c.exe
echo Building tileset akoi12
cd ..\akoi12
..\..\port\windows\%1\tile2c.exe
echo Building tileset akoi3x
cd ..\akoi3x
..\..\port\windows\%1\tile2c.exe
echo Building tileset ibsongrey
cd ..\ibsongrey
..\..\port\windows\%1\tile2c.exe
echo Building tileset lomaka
cd ..\lomaka
..\..\port\windows\%1\tile2c.exe
popd
goto exit

:error
echo Usage: prebuild_bitmaps.bat (Debug^|Release)
:exit
