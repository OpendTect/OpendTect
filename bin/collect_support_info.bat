ECHO OFF
::______________________________________________________________________________
::
:: This script collects information on the OpendTect installation setup,
:: the data area and the system on which it is run.
::
:: CVS: $Id: collect_support_info.bat,v 1.2 2004-10-08 14:52:47 dgb Exp $
::______________________________________________________________________________
::


:: Should be set by installer
cd %DTECT_WINAPPL%

:: Get cygwin directory, if installed
if exist .\bin\win\GetCygdir.exe goto getcygdir

 :: else

:nocygwin

 for /F "tokens=*" %%i in ('cd') do set CYGWINDIR=%%i
 set CYGWINDIR=%CYGWINDIR%\bin\win\sys
 goto doit

:getcygdir

 for /F "tokens=*" %%i in ('.\bin\win\GetCygdir.exe') do set CYGWINDIR=%%i
 set CYGWINDIR=%CYGWINDIR%\bin

 if exist %CYGWINDIR%\tcsh.exe goto doit
 goto nocygwin

:doit

set PATH=%CYGWINDIR%;%DTECT_WINAPPL%\bin\win\sys;%PATH%
:: the cygwin stuff is now in our PATH


:: =============================================================================
:: Determine temporary directory for storing output file
:: =============================================================================

if "%USERPROFILE%"=="" goto tmp
set outfil=%USERPROFILE%\supportinfo.txt
goto printoutfile

:tmp
if "%TMP%"=="" goto temp
set outfil=%TMP%\supportinfo.txt
goto printoutfile

:temp
if "%TEMP%"=="" goto notemp 
set outfil=%TEMP%\supportinfo.txt
goto printoutfile

:notemp
set outfil=C:\supportinfo.txt

:printoutfile
echo ===========================================================================
echo                            OpendTect Support Info
echo                            ----------------------
echo. 
echo   Collecting data. Output will be in: 
echo. 
echo           %outfil%
echo. 
echo ===========================================================================


echo ====================================================== > "%outfil%"
echo  OpendTect Support Info file. >> "%outfil%"
echo ====================================================== >> "%outfil%"
echo. >> "%outfil%"


echo                       --------- >> "%outfil%"
echo ====================================================== >> "%outfil%"
echo  DOS/CMD Environment >> "%outfil%"
echo ------------------------------------------------------ >> "%outfil%"
echo                       --------- >> "%outfil%"
set >> "%outfil%"


echo                       --------- >> "%outfil%"
echo ====================================================== >> "%outfil%"
echo  Calling csh script >> "%outfil%"
echo ------------------------------------------------------ >> "%outfil%"
echo                       --------- >> "%outfil%"

tcsh.exe "%DTECT_WINAPPL%\bin\win\collect_support_info.csh" "%outfil%"

unix2dos -q "%outfil%"

echo ===========================================================================
echo Done. Support Info written to:
echo. 
echo           %outfil%
echo. 
echo ===========================================================================

pause
