@echo off
if !%1==! goto error

echo Prebuilding text files...
pushd .
cd ..\..\..
port\windows\%1\txt2c.exe LICENSE.TXT license.cpp
port\windows\%1\txt2c.exe CREDITS.TXT credits.cpp
popd
goto exit

:error
echo Usage: prebuild_bitmaps.bat (Debug^|Release)
:exit
