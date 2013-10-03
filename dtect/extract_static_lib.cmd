@echo off
REM Extracts all objects from a static library and dumps them in the current
REM folder.
REM
REM Syntax: extract_static_lib <static.lib> <platform=win32|win64>
REM
REM $id$
REM

set staticlib=%1
set memberlist=memberlist.txt
set plf=%2

if "%plf%"=="win64" set vcplf=amd64
if "%plf%"=="win32" set vcplf=x86

call "c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" %vcplf%

lib /NOLOGO /LIST %staticlib% > %memberlist%

for /f "delims=" %%x in (%memberlist%) do lib /NOLOGO /EXTRACT:%%x "%staticlib%"

del %memberlist%
