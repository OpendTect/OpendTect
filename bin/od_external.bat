@ECHO OFF
@SETLOCAL

Set argC=0
For %%x in (%*) do Set /A argC+=1

if %argC% LSS 1 (
  echo Usage: command [ARGS]
  echo Set OD_INTERNAL_CLEANPATH to use a locally different path
  goto :eof
)
if DEFINED OD_INTERNAL_CLEANPATH (
  Set "PATH=%OD_INTERNAL_CLEANPATH%"
)

Set DTECT_APPL=
if DEFINED HOMEDRIVE if DEFINED HOMEPATH if EXIST %HOMEDRIVE%%HOMEPATH% (
  chdir /D %HOMEDRIVE%%HOMEPATH%
  GOTO :SUCCESS_END
)

:DEFAULT_HOME
if DEFINED USERNAME if EXIST C:\Users\%USERNAME% (
  chdir /D C:\Users\%USERNAME%
  GOTO :SUCCESS_END
)

:DEFAULT_DRIVE
if EXIST C:\ (
  chdir /D C:\
)

:SUCCESS_END
@CALL %*

ENDLOCAL
