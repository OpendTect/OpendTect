::@echo off
::
:: OpenTect remote startup script for window<->windows using rcmd
:: $Id: od_dormt.bat,v 1.2 2004-10-05 11:29:39 dgb Exp $
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
set tmpvar=%string:#-#= %
for /F "tokens=*" %%i in ('cygpath -wa "%tmpvar%"') do set ARGFILE=%%i

set string=%2
set tmpvar=%string:#-#= %
for /F "tokens=*" %%i in ('cygpath -wa "%tmpvar%"') do set PROCDIR=%%i

set datahost=%3
set datadrive=%4
set datashare=%5
set username=%6
set userpass=%7


::echo net use "\\%datahost%" . "/USER:%datahost%\%username%"
net use "\\%datahost%" %userpass% "/USER:%datahost%\%username%"

::echo net use %datadrive%: "\\%dathost%\%datashare%"
net use %datadrive%: "\\%datahost%\%datashare%"

if exist "%ARGFILE%" goto getargs

 :: else

 sleep 10

if exist "%ARGFILE%" goto getargs

echo Error: %ARGFILE% does not exist
exit 1

:getargs
:for /F "tokens=*" %%i in ('type "%ARGFILE%"') do set ARGS=%%i


tcsh.exe -f .\bin\od_do_rmt_file %ARGFILE%
