#!/bin/csh 
#
# Copyright (C): dGB Beheer B. V.
#

setenv LANG C
setenv  LC_CTYPE C
set progname = $0
set exceptionfile=""
set listfile=""
set keyword=""

if ( $#argv<2 ) then
    goto syntax
endif

nextarg:

if ( "${1}" == "--keyword" ) then
    shift
    set keyword = "${1}"
else if ( "${1}" == "--exceptionfile" ) then
    set exceptionfile=${2}
    if ( "${exceptionfile}" == "" ) then
	goto syntax
    endif

    if ( ! -e "${exceptionfile}" ) then
	echo Exception file \"${exceptionfile}\" does not exist.
	exit 1
    endif

    shift
else if ( "${1}" == "--listfile" ) then
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
    if ( "${exceptionfile}" != "" ) then
	cat ${listfile} | xargs -P 0 -n 200 ${progname} --keyword "${keyword}" --exceptionfile ${exceptionfile}
	exit ${status}
    else
	cat ${listfile} | xargs -P 0 -n 200 ${progname} --keyword "${keyword}"
	exit ${status}
    endif
endif

set failure = 0

nextfile:

set filename = ${1}

if ( "${filename}" == "" ) then
    exit ${failure}
endif

shift 

cat ${filename} | sed '{:q;N;s/\n/ /g;t q}' | egrep -q "${keyword}"
if ( ${status} == 0 ) then
    if ( "${exceptionfile}" != "" ) then
	grep -q ${filename} ${exceptionfile}
	if ( ${status} == 0 ) then
	    goto nextfile
	endif
    endif
	    
    echo ${filename}
    set failure = 1;
endif

goto nextfile

syntax:
echo
echo ${progname} - Finds the presens of keyword in files, unless they are
echo present in an exceptionfile.
echo
echo Returns 0 if keyword is not found, otherwise 1
echo
echo "Syntax ${progname} --keyword <kw> [--exceptionfile <file>] <--listfile <listfile> | files ..>"
echo
exit 1
