@echo off

call "odvars.bat"
"%VSEXE%" /useenv %WORK%\%MSVCDIR%\OpendTect.sln
