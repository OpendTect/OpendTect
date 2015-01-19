@ECHO OFF

if "%1"=="" (
    echo %0 lupdate-executable source-dir binary-dir
    exit 1
)

if "%4"=="" (
    for /r %%v in (data\localizations\source\*.ts) do (
	CALL %0 %%~nv %1 %2 %3 )

    exit 0
)

SETLOCAL

SET SRC="%3\data\localizations\source\%1.ts"
SET DST="%4\data\localizations\%1.qm"

SET LRELEASE=%2

IF NOT EXIST %DST% (
    "%LRELEASE%" "%SRC%" -qm "%DST%"
    GOTO END
) 

for /F "Delims=" %%I In ('xcopy /DHYL %DST% %SRC% ^|Findstr /I "File"') Do set /a _Newer=%%I 2>Null
if %_Newer%==0 "%LRELEASE%" "%SRC%" -qm "%DST%"

:END

ENDLOCAL
