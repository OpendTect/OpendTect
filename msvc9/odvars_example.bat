rem OpendTect (work)
@echo off

rem Set variables below and rename to odvars.bat
rem This file will be called from start_msvc_od.bat
rem and from ODBuild.exe

echo Setting up an OpendTect environment ...

set OD_QTDIR=C:\Development\Qt\4.4.3
set OD_COINDIR=C:\Development\Coin-3.0.0
set OD_FFTWDIR=C:\Development\fftw-3.2-dll

set WORK=C:\Development\od
set DGBWORK=C:\Development\dgb

rem Most probably no changes required from here
set DTECT_APPL=%WORK%
set MSVCDIR=msvc9
set PATH=%OD_QTDIR%\bin;%OD_COINDIR%\bin;%OD_FFTWDIR%;%PATH%

set VSDIR=C:\Program Files\Microsoft Visual Studio 9.0
set VSEXE=%VSDIR%\Common7\IDE\VCExpress.exe

call "%VSDIR%\Common7\Tools\vsvars32.bat"
