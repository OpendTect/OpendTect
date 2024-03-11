cls
@ECHO OFF

set inpdir=%1
set outdir=%2

if not exist %inpdir% goto MSGINPDIR
if not exist %inpdir%\relinfo\ver.devel_win64.txt if not exist %inpdir%\relinfo\ver.devel_win32.txt goto MSGPKG
if not exist %outdir% md %outdir%

xcopy /s /y %inpdir%\doc\Programmer\pluginexample\* %outdir%\
if exist %outdir%\plugins\win32 rd /S /Q %outdir%\plugins\win32\libs 
if exist %outdir%\plugins\win64 rd /S /Q %outdir%\plugins\win64\libs
if not exist "%outdir%\plugins" (
    xcopy /s /i /y %inpdir%\plugins\Tut\CMakeLists*.txt %outdir%\plugins\Tut
    xcopy /s /i /y %inpdir%\plugins\Tut\*.h %outdir%\plugins\Tut
    xcopy /s /i /y %inpdir%\plugins\Tut\*.cc %outdir%\plugins\Tut
    xcopy /s /i /y %inpdir%\plugins\uiTut\CMakeLists*.txt %outdir%\plugins\uiTut
    xcopy /s /i /y %inpdir%\plugins\uiTut\*.h %outdir%\plugins\uiTut
    xcopy /s /i /y %inpdir%\plugins\uiTut\*.cc %outdir%\plugins\uiTut
)
goto END


:MSGINPDIR
echo %inpdir% does not exist
goto END

:MSGPKG
echo warning : %inpdir% does not have the development package installed
goto END


:END
set inpdir=%inpdir:\=/%
set inpdir=%inpdir:"=%
echo OpendTect_DIR:PATH=%inpdir% > %outdir%\CMakeCache.txt
echo Done
