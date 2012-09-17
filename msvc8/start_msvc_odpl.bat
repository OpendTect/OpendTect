@echo off

call "odvars.bat"
"%VSEXE%" /useenv %WORK%\%MSVCDIR%\OpendTectPlugins.sln
