@ECHO OFF

if "%1"=="" (
    echo %0 lrelease-executable source-dir binary-dir
    exit 1
)

REM If main call, find the files and do it with the indididual files
if "%4"=="" (
    cd %2
    for /r %%v in (\data\localizations\source\*.ts) do (
	CALL %0 %%~nv %1 %2 %3
	if errorlevel 1 (
	   echo Compilation of %%~nv failed
	   exit /b %errorlevel%
	)
    )

    exit /b 0
)

SETLOCAL

SET SRC="%3\data\localizations\source\%1.ts"
SET DST="%4\data\localizations\%1.qm"
SET LRELEASE=%2

REM If not done at all, just do it!
IF NOT EXIST %DST% (
    "%LRELEASE%" -compress "%SRC%" -qm "%DST%"
    GOTO END
) 

REM Check if it is needed, then do it
for /F "Delims=" %%I In ('xcopy /DHYL %DST% %SRC% ^|Findstr /I "File"') Do set /a _Newer=%%I 2>Null
if %_Newer%==0 (
    "%LRELEASE%" -compress "%SRC%" -qm "%DST%"
)

:END
REM Since lrelease returns non-zero if there are any non-translated strigns,
REM we won't return that value. It may change one day though.
exit /b 0

ENDLOCAL
