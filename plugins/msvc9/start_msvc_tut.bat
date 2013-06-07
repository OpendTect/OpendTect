@echo off

call "odvars.bat"
"%VSEXE%" /useenv %WORK%\plugins\msvc9\Tutorials.sln
