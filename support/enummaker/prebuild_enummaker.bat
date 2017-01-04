@echo off
if !%1==! goto error

echo Prebuilding enums...
pushd .
cd ..\..\..
port\windows\%1\enummaker.exe source.txt
popd
goto exit

:error
echo Usage: prebuild_bitmaps.bat (Debug^|Release)
:exit
