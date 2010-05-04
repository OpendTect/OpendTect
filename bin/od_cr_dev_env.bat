cls
@echo off

set inpdir=%1
set outdir=%2

if not exist %inpdir% goto MSGINPDIR
if not exist %inpdir%\.rel.devel goto MSGPKG
if not exist %outdir% goto MSGOUTDIR


FOR %%a IN ( plugins, src, include, Pmake, spec, msvc8 ) DO md %outdir%\%%a
FOR %%a IN ( plugins, src, include, Pmake, spec, msvc8 ) DO xcopy /s %inpdir%\%%a\* %outdir%\%%a

copy %inpdir%\.rel.devel %outdir%
goto END


:MSGINPDIR
echo %inpdir% does not exist
goto END

:MSGOUTDIR
echo %outdir% does not exist
goto END

:MSGPKG
echo warning : %inpdir% does not have the development package installed
goto END


:END
echo Done
pause
