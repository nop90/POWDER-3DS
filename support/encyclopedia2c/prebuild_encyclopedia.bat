@echo off
if !%1==! goto error

echo Prebuilding encyclopedia...
..\%1\encyclopedia2c.exe ..\..\..\encyclopedia.txt
goto exit

:error
echo Usage: prebuild_bitmaps.bat (Debug^|Release)
:exit
