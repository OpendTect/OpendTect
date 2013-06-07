@echo off

call "odvars.bat"
"%VSEXE%" /useenv %WORK%\plugins\%MSVCDIR%\Tutorials.sln
