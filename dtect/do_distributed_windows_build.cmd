@echo off
SETLOCAL
set PATH=%PATH%;C:\appman\apps\ninja\

buildconsole /COMMAND="ninja -j70"

ENDLOCAL

