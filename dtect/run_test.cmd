@ECHO OFF
REM 
REM Copyright (C) 2012- : dGB Beheer B. V.
REM
REM

setlocal

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
) ELSE IF "%1"=="--plf" (
    set plf=%2
    shift
) ELSE IF "%1"=="--qtdir" (
    set qtdir=%2
    shift
) ELSE ( goto do_it )

shift
goto parse_args

:syntax
echo run_test --command cmd --wdir workdir --plf platform --config config --qtdir qtdir
exit 1

:do_it

IF NOT DEFINED cmd ( echo --command not specified.
		    	goto syntax )
IF NOT DEFINED plf ( echo --plf not specified.
		    	goto syntax )
IF NOT DEFINED config ( echo --config not specified.
			goto syntax )
IF NOT DEFINED wdir ( echo --wdir not specified.
			goto syntax )
IF NOT DEFINED qtdir ( echo --qtdir not specified.
			goto syntax )

set qtdir=%qtdir:/=\%
set bindir=%wdir%/bin/%plf%/%config%
set bindir=%bindir:/=\%

if NOT EXIST "%bindir%" ( 
    echo %bindir% does not exist!
    exit 1
)

set fullcommand=%bindir%\%cmd%
if NOT EXIST "%fullcommand%" ( 
    echo %fullcommand% does not exist!
    exit 1
)

set PATH=%bindir%;%qtdir%/bin;%PATH%
"%fullcommand%"
IF %errorlevel% NEQ 0 (
   echo %fullcommand% returned %errorlevel%
   exit /b %errorlevel%
)
exit /b 0
