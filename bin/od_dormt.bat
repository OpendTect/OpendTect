@echo off
::
:: OpenTect remote startup script for window<->windows using rcmd
:: $Id: od_dormt.bat,v 1.8 2004-11-18 11:11:31 dgb Exp $
::______________________________________________________________________________

cd %DTECT_WINAPPL%


:: Get cygwin directory, if installed
if exist .\bin\win\GetCygdir.exe goto getcygdir

 :: else

:nocygwin

 for /F "tokens=*" %%i in ('cd') do set CYGWINDIR=%%i
 set CYGWINDIR=%CYGWINDIR%\bin\win\sys
 goto doit

:getcygdir

 for /F "tokens=*" %%i in ('.\bin\win\GetCygdir.exe') do set CYGWINDIR=%%i
 set CYGWINDIR=%CYGWINDIR%\bin

 if exist %CYGWINDIR%\tcsh.exe goto doit
 goto nocygwin

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
