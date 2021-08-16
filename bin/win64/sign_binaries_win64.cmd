REM Script to sign DLLs and EXEs files.

@ECHO OFF
SETLOCAL enabledelayedexpansion

set sigexe=C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\signtool.exe
set tim=http://rfc3161timestamp.globalsign.com/advanced

set argCount=0
for %%x in (%*) do (
   set /A argCount+=1
   set "argVec[!argCount!]=%%~x"
)

for /L %%i in (1,1,%argCount%) do (
    echo %%i- "!argVec[%%i]!"
    set dirname=%%i

    FOR /F %%b in ( 'dir /s /b !argVec[%%i]!\*.exe !argVec[%%i]!\*.dll' ) DO ( 
	set counter=0
	set maxcount=5
	set filename=%%b
	"%sigexe%" verify /pa !filename! > NUL
	IF !ERRORLEVEL! GEQ 1 (
	    :resignfile
	    echo signing !filename!
	    "%sigexe%" sign /a /tr %tim% /fd SHA256 /td SHA256 !filename!
	    IF %ERRORLEVEL% GEQ 1 (
		set /A counter=%counter%+1
		IF !counter! LEQ !maxcount! (
		    echo Trying to sign !filename! again !counter! times
		    goto resignfile
		) ELSE (
		    echo Tried !maxcount! times to sign !filename!
		    goto ERROR_END
		)
	    )
	)
    )
)

goto SUCCESS_END

:ERROR_END
echo Signing failed
exit /b %ERROR_LEVEL%

:SUCCESS_END
echo Signing success

ENDLOCAL
