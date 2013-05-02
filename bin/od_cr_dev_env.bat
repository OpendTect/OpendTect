cls
@echo off

set inpdir=%1
set outdir=%2

if not exist %inpdir% goto MSGINPDIR
if not exist %inpdir%\relinfo\ver.devel_win64.txt if not exist %inpdir%\relinfo\ver.devel_win32.txt goto MSGPKG
if not exist %outdir% goto MSGOUTDIR

md %outdi%
xcopy /s %inpdir%\doc\Programmer\pluginexample\* %outdir%\
if exist %outdir%\plugins\win32 rd /S /Q %outdir%\plugins\win32\libs 
if exist %outdir%\plugins\win64 rd /S /Q %outdir%\plugins\win64\libs 
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
