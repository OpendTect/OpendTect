@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

SET argC=0
FOR %%x IN (%*) DO SET /A argC+=1

IF %argC% LSS 1 (
    echo Usage: command [ARGS]
    echo Set OD_INTERNAL_CLEANPATH to use a locally different path
    GOTO :eof
)

SET DTECT_APPL=
IF DEFINED HOMEDRIVE IF DEFINED HOMEPATH IF EXIST %HOMEDRIVE%%HOMEPATH% (
    chdir /D %HOMEDRIVE%%HOMEPATH%
    Set "WindowsAppDir=%HOMEDRIVE%%HOMEPATH%\AppData\Local\Microsoft\WindowsApps"
    IF EXIST %WindowsAppDir%\python.exe (
	GOTO :SET_PATH
    )
    IF EXIST %WindowsAppDir%\python3.exe (
	GOTO :SET_PATH
    )
    Set WindowsAppDir=
    GOTO :SET_PATH
)

:DEFAULT_HOME
IF DEFINED USERNAME IF EXIST C:\Users\%USERNAME% (
    chdir /D C:\Users\%USERNAME%
    GOTO :SET_PATH
)

:DEFAULT_DRIVE
IF EXIST C:\ chdir /D C:\

:SET_PATH
IF DEFINED WindowsAppDir (
    IF DEFINED OD_INTERNAL_CLEANPATH (
	SET "PATH=!OD_INTERNAL_CLEANPATH:%WindowsAppDir%=!"
    ) ELSE (
	SET "PATH=!PATH:%WindowsAppDir%=!"
    )
) ELSE (
    IF DEFINED OD_INTERNAL_CLEANPATH SET "PATH=%OD_INTERNAL_CLEANPATH%"
)

@CALL %*

ENDLOCAL
