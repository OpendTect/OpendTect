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

if %nrargs% LSS 4 (
    echo %0
    echo  Since processing a minidump on Windows doesn't give desired results
    echo  most of the time, we send the minidump file itself on Windows.
    echo  launch a sender applicaiton with the minidump file as argument.
    echo Usage : %0 ^<dumpfile^> ^<symbol-dir^> ^<resolve application^> 
    echo ^<archive-prefix^> [sender] [args to sender]
    exit /b 1
)

REM Extract the input parameters
set dumpfile=%1
set tmpdir=%~dp1
shift

set symboldir=%1
shift

set dumphandler=%1
shift

set archivename=%1
shift

set sender=
if %nrargs% GTR 3 (
    set sender=%1
    shift
)


REM Check the input parameters
if not exist %dumpfile% (
    echo %dumpfile% does not exist
    exit /b 1
)

REM Send the minidump-file
if exist "%sender%" (
    %sender% %dumpfile% --binary 
)

REM Cleanup
REM del %dumpfile%

exit /b 0
