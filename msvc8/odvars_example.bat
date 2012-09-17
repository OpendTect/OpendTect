rem OpendTect (work)
@echo off

rem Set variables below and rename to odvars.bat
rem This file will be called from start_msvc_od.bat
rem and from ODBuild.exe

echo Setting environment for building OpendTect ...

set OD_QTDIR=C:\Development\Qt\4.4.3
set OD_COINDIR=C:\Development\Coin-3.0.0
set OD_FFTWDIR=C:\Development\FFTW\v3.2.2

set WORK=C:\Development\od
set DGBWORK=C:\Development\dgb

rem Most probably no changes required from here
set DTECT_APPL=%WORK%
set PATH=%OD_QTDIR%\bin;%OD_COINDIR%\bin;%OD_FFTWDIR%;%PATH%
set MSVCDIR=msvc

set VSDIR=C:\Program Files\Microsoft Visual Studio 8
set VSEXE=devenv

call "%VSDIR%\Common7\Tools\vsvars32.bat"
