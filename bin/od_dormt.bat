@echo off
::
:: OpenTect remote startup script for window<->windows using rcmd
:: $Id: od_dormt.bat,v 1.9 2005-04-06 13:36:14 cvsarend Exp $
::______________________________________________________________________________

cd %DTECT_WINAPPL%

set HDIR=win

:: Get dir of cygwin utils shipped with OpendTect
for /F "tokens=*" %%i in ('cd') do set ODSYSDIR=%%i
set ODSYSDIR=%ODSYSDIR%\bin\win\sys
set PATH=%ODSYSDIR%;%ODSYSDIR%\dlls;%PATH%

set CSHEXE=tcsh.exe
set CSHCMD=tcsh -f

:: Get cygwin directory, if installed
if not exist .\bin\win\GetCygdir.exe goto doit

 for /F "tokens=*" %%i in ('.\bin\win\GetCygdir.exe') do set CYGWINDIR=%%i
 set CYGWINDIR=%CYGWINDIR%\bin

 if not exist %CYGWINDIR% goto doit

  set PATH=%CYGWINDIR%;%PATH%

:doit

set PATH=%CYGWINDIR%;%PATH%

:: Now do what we setup to do

set string=%1

set datahost=%2
set datadrive=%3
set datashare=%4
set username=%5
set userpass=%6

for /F "tokens=*" %%i in ('.\bin\win\SearchODFile.exe od_prepare.bat') do set PRESCRIPT=%%i

cmd /c "%PRESCRIPT%" %datahost% %datadrive% %datashare% %username% %userpass%

:: Use cygpath only after prescipt has executed and the data dir is mounted.
set tmpvar=%string:#-#= %
for /F "tokens=*" %%i in ('cygpath -wa "%tmpvar%"') do set ARGFILE=%%i

if exist "%ARGFILE%" goto getargs

 :: else

 sleep 10

if exist "%ARGFILE%" goto getargs

echo Error: %ARGFILE% does not exist
exit 1

:getargs
tcsh.exe -f .\bin\od_do_rmt_file %ARGFILE%
