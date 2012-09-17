rem OpendTect (work)
@echo off

rem Set variables below and rename to odvars.bat
rem This file will be called from start_msvc_od.bat
rem and from ODBuild.exe

echo Setting environment for building OpendTect ...

set WORK=C:\dw105\dev\od
set DGBWORK=C:\dw105\dev\dgb
set DTECT_APPL=%WORK%

SET OSGDIR=C:\dw105\OpenSceneGraph-3.0.1
SET COINDIR=C:\dw105\coin-od\Coin-3.1.3
SET QTDIR=C:\dw105\Qt\4.5.3


set OD_COINDIR=%COINDIR%
set OD_QTDIR=%QTDIR%
set OD_OSGDIR=%OSGDIR%
set MSVCDIR=msvc10
set PATH=%OD_COINDIR%\bin;%OD_QTDIR%\bin;%PATH%

call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86_amd64
set VSEXE=devenv


rem  ----------------- FOR USING EXPRESS EDITION-----------------------
rem
rem  please replace the above two lines with the lines written below.
rem  ------------------------------------------------------------------
rem  set VSDIR=C:\Program Files (x86)\Microsoft Visual Studio 10.0
REM  set VSEXE=VCExpress

REM call "%VSDIR%\Common7\Tools\vsvars32.bat"
REM --------------------------------------------------------------------

