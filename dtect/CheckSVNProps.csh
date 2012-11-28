#!/bin/csh 
#
# Copyright (C): dGB Beheer B. V.
#

set progname = $0
set exceptionfile=""
set listfile=""

if ( $#argv<2 ) then
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

set tmpfile=/dev/shm/cksvnprop.$$

nextfile:

set filename = ${1}

if ( "${filename}" == "" ) then
    rm -rf ${tmpfile}
    exit 0
endif

shift 

(svn proplist ${filename} > ${tmpfile} ) >& /dev/null
if ( ${status} == 0 ) then
    cat ${tmpfile} | grep -q eol-style
    if ( ${status} == 1 ) then
	echo File ${filename} misses eol-style.
	rm -rf ${tmpfile}
	exit 1
    endif
    cat ${tmpfile} | grep -q keyword
    if ( ${status} == 1 ) then
	echo File ${filename} misses keyword.
	rm -rf ${tmpfile}
	exit 1
    endif
endif

goto nextfile

syntax:
echo
echo ${progname} - Finds files without the svn:eol-style set
echo
echo Returns 0 if keyword is not found, otherwise 1
echo
echo "Syntax ${progname}  <--listfile <listfile> | files ..>"
echo
exit 1
