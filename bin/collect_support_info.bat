@ECHO OFF
::______________________________________________________________________________
::
:: This script collects information on the OpendTect installation setup,
:: the data area and the system on which it is run.
::
:: CVS: $Id: collect_support_info.bat,v 1.4 2004-12-09 10:20:28 dgb Exp $
::______________________________________________________________________________
::

if %DTECT_WINAPPL%No == No goto nodtectappl

:: Should be set by installer
cd %DTECT_WINAPPL%

:nodtectappl

:: Get dir of cygwin utils shipped with OpendTect
for /F "tokens=*" %%i in ('cd') do set ODSYSDIR=%%i
set ODSYSDIR=%ODSYSDIR%\bin\win\sys
set PATH=%ODSYSDIR%;%PATH%

set CSHEXE=tcsh.exe
set CSHCMD=tcsh -f

:: Get cygwin directory, if installed
if not exist .\bin\win\GetCygdir.exe goto doit

 for /F "tokens=*" %%i in ('.\bin\win\GetCygdir.exe') do set CYGWINDIR=%%i
 set CYGWINDIR=%CYGWINDIR%\bin

 if not exist %CYGWINDIR% goto doit

  set PATH=%CYGWINDIR%;%PATH%

:doit


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

%CSHCMD% "%DTECT_WINAPPL%\bin\win\collect_support_info.csh" "%outfil%"

unix2dos -q "%outfil%"

echo ===========================================================================
echo Done. Support Info written to:
echo. 
echo           %outfil%
echo. 
echo ===========================================================================

pause
