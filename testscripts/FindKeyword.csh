#!/bin/csh 
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

setenv LANG C
setenv  LC_CTYPE C
set progname = $0
set exceptionfile=""
set listfile=""
set keyword=""
set message="Keyword found"
set grepcommand=grep

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
else if ( "${1}" == "--message" ) then
    set message = "${2}"
    if ( "${message}" == "" ) then
	goto syntax
    endif
    shift
else if ( "${1}" == "--grepcommand" ) then
    set grepcommand = "${2}"
    if ( "${grepcommand}" == "" ) then
	goto syntax
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
	cat ${listfile} | xargs -P 0 -n 200 ${progname} --keyword "${keyword}" --exceptionfile ${exceptionfile} --message "${message}" --grepcommand "${grepcommand}"
	exit ${status}
    else
	cat ${listfile} | xargs -P 0 -n 200 ${progname} --keyword "${keyword}" --message "${message}" --grepcommand "${grepcommand}"
	exit ${status}
    endif
endif

set failure = 0

nextfile:

set filename = ${1}

if ( "${filename}" == "" ) then
    if ( ${failure} == 1 ) then
	echo ${message}
    endif
    exit ${failure}
endif

shift 

if ( -e ${filename} ) then
    cat ${filename} | \
	# Remove // comments \
	sed 's/\/\/.*//' | \
	# Make everything one line \
	sed '{:q;N;s/\n/ /g;t q}' | \
	# Remove /* */comments \
	sed 's/\/\*.*\*\///' | \
	#search for keyword \
	${grepcommand} -q "${keyword}"
    if ( ${status} == 0 ) then
	if ( "${exceptionfile}" != "" ) then
	    ${grepcommand} -q ${filename} ${exceptionfile}
	    if ( ${status} == 0 ) then
		goto nextfile
	    endif
	endif
		
	echo ${filename}
	set failure = 1;
    endif
endif

goto nextfile

syntax:
echo
echo ${progname} - Finds the presens of keyword in files, unless they are
echo present in an exceptionfile.
echo
echo Returns 0 if keyword is not found, otherwise 1
echo
echo "Syntax ${progname} --keyword <kw> [--message <msg>] [--exceptionfile <file>] <--listfile <listfile> | files ..>"
echo
exit 1
