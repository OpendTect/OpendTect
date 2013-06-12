#!/bin/csh 
#
# Copyright (C): dGB Beheer B. V.
#
# $Id$
#

set progname = $0
set exceptionfile=""
set listfile=""

if ( $#argv<2 ) then
    goto syntax
endif

if ( -e /dev/shm ) then
    set tmpdir=/dev/shm
else
    set tmpdir=/tmp
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
    cat ${listfile} | grep \\.ico -v | xargs ${progname} 
    exit ${status}
endif

set tmpfile=${tmpdir}/cksvnprop.$$
set tmperrfile=${tmpdir}/cksvnprop_err.$$
set exitcode = 0

nextfile:

set filename = ${1}

if ( "${filename}" == "" ) then
    rm -rf ${tmpfile} ${tmperrfile}

    if ( ${exitcode} == 1 ) then
	echo "Fails test."
    endif

    exit ${exitcode}
endif

shift 

(svn proplist ${filename} > ${tmpfile} ) >& ${tmperrfile}
set errsize=`ls -la ${tmperrfile} | awk '{ print $5 }'`
if ( ${errsize} == 0 ) then
    set localfail = 0
    cat ${tmpfile} | grep -q "svn:eol-style"
    if ( ${status} == 1 ) then
	echo ${filename}
	set localfail=1
    endif
    cat ${tmpfile} | grep -q "svn:keyword"
    if ( ${localfail} == 0 && ${status} == 1 ) then
	echo ${filename}
	set localfail=1
    endif
    cat ${tmpfile} | grep -q "svn:needs-lock"
    if ( ${localfail} == 0 && ${status} == 0 ) then
	echo ${filename}
	set localfail=1
    endif

    if ( ${localfail} == 1 ) then
	set exitcode=1
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
