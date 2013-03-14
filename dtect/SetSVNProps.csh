#!/bin/csh 
#
# Copyright (C): dGB Beheer B. V.
#
#
# $Id$

set progname = $0
set exceptionfile=""
set listfile=""

if ( $#argv<1 ) then
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
set errsize=`ls -la ${tmperrfile} | awk '{ print $5}'`
if ( ${errsize} == 0 ) then
    svn propset svn:keywords "Author Date Id Revision URL" ${filename}
    if ( ${status} != 0 ) then
	echo "Cannot set keyword property on ${filename}"
	rm -rf ${tmpfile} ${tmperrfile}
	exit 1
    endif
	
    svn propset svn:eol-style "native" ${filename}
    if ( ${status} != 0 ) then
	echo "Cannot set eol-style property on ${filename}"
	rm -rf ${tmpfile} ${tmperrfile}
	exit 1
    endif
endif

goto nextfile

syntax:
echo
echo ${progname} - Sets the standard svn properties for source files
echo
echo Returns 0 if success, otherwise 1
echo
echo "Syntax ${progname}  <--listfile <listfile> | files ..>"
echo
exit 1
