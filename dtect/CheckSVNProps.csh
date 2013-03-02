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
    cat ${listfile} | xargs ${progname} 
    exit ${status}
endif

set tmpfile=${tmpdir}/cksvnprop.$$
set tmperrfile=${tmpdir}/cksvnprop_err.$$

nextfile:

set filename = ${1}

if ( "${filename}" == "" ) then
    rm -rf ${tmpfile} ${tmperrfile}
    exit 0
endif

shift 

(svn proplist ${filename} > ${tmpfile} ) >& ${tmperrfile}
set errsize=`stat -c %s ${tmperrfile}`
if ( ${errsize} == 0 ) then
    cat ${tmpfile} | grep -q "svn:eol-style"
    if ( ${status} == 1 ) then
	echo File ${filename} misses svn:eol-style.
	rm -rf ${tmpfile} ${tmperrfile}
	exit 1
    endif
    cat ${tmpfile} | grep -q "svn:keyword"
    if ( ${status} == 1 ) then
	echo File ${filename} misses keyword.
	rm -rf ${tmpfile} ${tmperrfile}
	exit 1
    endif
    cat ${tmpfile} | grep -q "svn:needs-lock"
    if ( ${status} == 0 ) then
	echo File ${filename} has lock property set.
	rm -rf ${tmpfile} ${tmperrfile}
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
