@ECHO OFF
REM ________________________________________________________________________
REM Copyright:	(C) 1995-2022 dGB Beheer B.V.
REM License:	https://dgbes.com/licensing
REM ________________________________________________________________________

setlocal
set args=
set "pathdirs="
set expret=0

:parse_args
IF "%1"=="--command" (
    set cmd=%2
    shift
) ELSE IF "%1"=="--config" (
    set config=%2
    shift
) ELSE IF "%1"=="--wdir" (
    set wdir=%2
    shift
) ELSE IF "%1"=="--quiet" (
    set args=%args% --quiet
) ELSE IF "%1"=="--parfile" (
    set args=%args% %2
    shift
) ELSE IF "%1"=="--expected-result" (
    set expret=%2
    shift
) ELSE IF "%1"=="--plf" (
    set plf=%2
    shift
) ELSE IF "%1"=="--datadir" (
    set args=%args% --datadir %2
    shift
) ELSE IF "%1"=="--pathdirs" (
    IF "%pathdirs%" == "" (
        set "pathdirs=%~2"
    ) ELSE (
        set "pathdirs=%pathdirs%;%~2"
    )
    shift
) ELSE ( goto do_it )

shift
goto parse_args

:syntax
echo run_test --command cmd --wdir workdir --plf platform --config config --pathdirs dirs --datadir datadir --parfile parfile --expected-return expected-return
exit /b 1

:do_it

IF NOT DEFINED cmd ( echo --command not specified.
		    	goto syntax )
IF NOT DEFINED plf ( echo --plf not specified.
		    	goto syntax )
IF NOT DEFINED config ( echo --config not specified.
			goto syntax )
IF NOT DEFINED wdir ( echo --wdir not specified.
			goto syntax )

set pathdirs=%pathdirs:/=\%
set bindir=%wdir%/bin/%plf%/%config%
set bindir=%bindir:/=\%

if NOT EXIST "%bindir%" ( 
    echo %bindir% does not exist!
    exit /b 1
)

set fullcommand=%bindir%\%cmd%.exe
if NOT EXIST "%fullcommand%" ( 
    echo %fullcommand% does not exist!
    exit /b 1
)

set PATH=%bindir%;%pathdirs%;%PATH%

"%fullcommand%" %args%

IF %errorlevel% NEQ %expret% (
   echo %fullcommand% returned %errorlevel%, while expecting %expret%
   exit /b 1
)
exit /b 0
