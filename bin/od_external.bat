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
  Set PATH="%OD_INTERNAL_CLEANPATH%"
)

Set DTECT_APPL=
@CALL %*
