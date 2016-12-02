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
    echo  tries to resolve a minidump file using symbols in a symbol
    echo  directory, save it as a text file, and optionally
    echo  launch a sender applicaiton with the text-file as argument.
    echo.
    echo The dump-file will be deleted
    echo
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

if not exist "%symboldir%" (
    echo %symboldir% does not exist
    exit /b 1
)

if not exist "%dumphandler%" (
    echo %dumphandler% does not exist
    exit /b 1
)

set tmpfile="%dumpfile%.tmp"
set textfile="%dumpfile%.txt"
set archivefile=%tmpdir%\%archivename%_%username%.txt
set logfile=%tmpdir%\%archivename%_%username%.log


if exist %tmpfile% (
    del %tmpfile%

    if exist %tmpfile% (
	echo %tmpfile% exists, and I cannot remove it."
	exit /b 1
    )
)


if exist %textfile% (
    del %textfile%

    if exist %textfile% (
	echo %textfile% exists, and I cannot remove it."
	exit /b 1
    )
)


REM Create the text-file (human readable)
%dumphandler% %dumpfile% %symboldir% 1> %tmpfile% 2>%logfile%

REM Create the text-file (machine readable)
echo Machine readable: >> %tmpfile%
%dumphandler% -m %dumpfile% %symboldir% 1>> %tmpfile% 2>%logfile%

REM Make DOS line endings 
type %tmpfile% | more /p > %textfile%

REM Send the text-file
if exist "%sender%" (
    %sender% %textfile% 
)

REM Make an archive copy of the report without timestamps so it can be picked up
REM Timestamps will only fill disks
copy %textfile% %archivefile% >NUL
if exist %archivefile% echo Error report saved as %archivefile%


REM Cleanup
del %dumpfile%
del %tmpfile%
del %textfile%

exit /b 0
