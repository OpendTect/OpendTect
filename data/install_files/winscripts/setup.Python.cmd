@ECHO OFF
REM
REM (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
REM AUTHOR   : Arjan
REM DATE     : Oct 2019
REM
REM
REM

set nrargs=0

for %%i in (%*) do set /A nrargs+=1

if %nrargs% LSS 1 (
    echo %0
    echo tries to create Python_envs.txt file in the OpendTect data dir
    echo
    echo Usage : %0 ^<odinstdir^> 
    exit /b 1
)

REM run script elevated if it is not
set winSysFolder=System32
NET file 1>nul 2>nul && goto :createtxtfile || powershell -ex unrestricted -Command "Start-Process -Verb RunAs -FilePath '%SystemRoot%\%winSysFolder%\cmd.exe' -ArgumentList '/c %~sfnx0 %~s1'"
goto :eof

:createtxtfile
REM Extract the input parameters
set odinstdir=%1
set condarootdir=%~dp0
shift

REM Check the input parameters
if not exist %odinstdir%\data (
    echo %odinstdir%\data does not exist
    exit /b 1
)

REM Create Python_envs.txt file
set fil=%odinstdir%\data\Python_envs.txt
echo dTect V6.6 > %fil%
echo Python >> %fil%
echo %DATE% %TIME% >> %fil%
echo ! >> %fil%
echo File name: %condarootdir% >> %fil%
echo ! >> %fil%

if exist %odinstdir%\v7\data (
    copy %fil% %odinstdir%\v7\data
)

:eof
exit /b 0
