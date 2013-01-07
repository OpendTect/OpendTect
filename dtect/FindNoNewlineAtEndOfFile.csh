#!/bin/csh 
#
# Copyright (C): dGB Beheer B. V.
#

set progname = $0
set exceptionfile=""
set listfile=""

if ( $#argv<1 ) then
    goto syntax
endif

nextarg:

if ( "${1}" == "--listfile" ) then
    set listfile=${2}
    if ( "${listfile}" == "" ) then
	goto syntax
    endif

    if ( ! -e "${listfile}" ) then
	echo List-file \"${listfile}\" does not exist.
	exit 1
    endif
    shift
else
   goto do_it
endif

shift
goto nextarg

do_it:

if ( "${listfile}" != "" ) then
    cat ${listfile} | xargs ${progname} 
    exit ${status}
endif

nextfile:

set filename = ${1}

if ( "${filename}" == "" ) then
    exit 0
endif

shift 

set char=`tail -c1 ${filename} | od -x | head -n 1 | awk '{print $2 }' `
if ( "${char}" != "000a" ) then
    echo "No EOL at end of ${filename}"
    exit 1
endif

goto nextfile

syntax:
echo
echo ${progname} - Checks that all files end with a new line
echo
echo Returns 0 if all files end with a newline, otherwise 1
echo
echo "Syntax ${progname} <--listfile <listfile> | files ..>"
echo
exit 1
