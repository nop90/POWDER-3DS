@echo off
if !%1==! goto error

echo Prebuilding bitmaps...
pushd .
cd ..\..\..\gfx
..\port\windows\%1\bmp2c.exe slug_and_blood.bmp
..\port\windows\%1\bmp2c.exe slug_and_blood_hires.bmp
..\port\windows\%1\bmp2c.exe icon_sdl.bmp
..\port\windows\%1\bmp2c.exe tridude_goodbye.bmp
..\port\windows\%1\bmp2c.exe tridude_goodbye_hires.bmp
popd
goto exit

:error
echo Usage: prebuild_bitmaps.bat (Debug^|Release)
:exit
