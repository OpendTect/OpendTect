@ECHO OFF
REM
REM (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
REM AUTHOR   : Kristofer
REM DATE     : Nov 2015
REM
REM
REM

set nrargs=0

for %%i in (%*) do set /A nrargs+=1

if %nrargs% LSS 2 (
    echo %0
    echo  Since processing a minidump on Windows doesn't give desired results
    echo  most of the time, we send the minidump file itself on Windows.
    echo  launch a sender application with the minidump file as argument.
    echo Usage : %0 ^<dumpfile^> [sender] [args to sender]
    exit /b 1
)

REM Extract the input parameters
set dumpfile=%1
shift

set sender=%1
shift

set senderargs=
:getappargs
if "%1"=="" goto after_loop
if "%senderargs%"=="" (
    set senderargs=%1
) else (
    set senderargs=%senderargs% %1
)
shift
goto getappargs

:after_loop
if "%senderargs%"=="" (
    set senderargs=--binary
)

REM Check the input parameters
if not exist %dumpfile% (
    echo %dumpfile% does not exist
    exit /b 1
)

%sender% %dumpfile% %senderargs%
exit /b 0
