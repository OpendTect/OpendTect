#!/bin/csh -f
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#
# Wrapper script to run a test
#

set datadir = ""
set wdir = ""
set plf = ""
set config = ""
set cmd = ""
set args = ""
set ldpathdir = ""
set expret = 0
set valgrind = ""
set gensuppressions="all"
set attachdebugger="yes"

parse_args:
if ( "$1" == "--command" ) then
    set cmd=$2
    shift
else if ( "$1" == "--datadir" ) then
    set datadir=$2
    shift
else if ( "$1" == "--plf" ) then
    set plf=$2
    shift
else if ( "$1" == "--oddir" ) then
    set args="${args} --oddir $2"
    shift
else if ( "$1" == "--quiet" ) then
    set args="${args} --quiet"
    set gensuppressions="no"
    set attachdebugger="no"
else if ( "$1" == "--valgrind" ) then
    set valgrind=$2
    shift
else if ( "$1" == "--expected-result" ) then
    set expret=$2
    shift
else if ( "$1" == "--wdir" ) then
    set wdir=$2
    shift
else if ( "$1" == "--config" ) then
    set config=$2
    shift
else if ( "$1" == "--parfile" ) then
    set args="${args} $2"
    shift
else if ( "$1" == "--ldpathdir" ) then
    set ldpathdir=$2
    shift
else if ( "$1" == "--help" ) then
    echo "Usage: run_test [option]"
    echo " --command cmd"
    echo " --ldpathdir ldpath"
    echo " --datadir datadir"
    echo " [--parfile parfile]"
    exit 0
else
    goto do_it
endif

shift
goto parse_args

do_it:

if ( "$cmd" == "" ) then
    echo "Command not specified"
    exit 1
endif

if ( "$wdir" == "" ) then
    echo "--wdir not specified"
    exit 1
endif

if ( "$plf" == "" ) then
    echo "--plf not specified"
    exit 1
endif

if ( "$config" == "" ) then
    echo "--config not specified"
    exit 1
endif

#Figure out libdir
set kernel=`uname -a | awk '{print $1}'`

if ( "${kernel}" == "Darwin" ) then
    set bindir="${wdir}/Contents/MacOS"
else
    set bindir="${wdir}/bin/${plf}/${config}"
endif

if ( "$ldpathdir" != "" ) then
    if ( $?LD_LIBRARY_PATH ) then
	setenv LD_LIBRARY_PATH ${ldpathdir}:${LD_LIBRARY_PATH}
    else
	setenv LD_LIBRARY_PATH ${ldpathdir}
    endif
    if ( $?DYLD_LIBRARY_PATH ) then
	setenv DYLD_LIBRARY_PATH ${ldpathdir}:${DYLD_LIBRARY_PATH}
    else
	setenv DYLD_LIBRARY_PATH ${ldpathdir}
    endif
endif

if ( "$datadir" != "" ) then
    set args = "${args} --datadir ${datadir}"
endif

if ( "${valgrind}" != "" ) then
    set suppression = `ls ${wdir}/testscripts/suppressions/*.supp | awk '{ print "--suppressions" "=" $1 }'`
    ${valgrind} \
	${suppression} \
	--gen-suppressions=${gensuppressions} \
	--db-attach=${attachdebugger} \
	"-q" \
	"--tool=memcheck" \
	"--leak-check=full" \
	"--show-reachable=no" \
	"--workaround-gcc296-bugs=yes" \
	"--num-callers=50" \
	"--track-origins=yes" \
	"--error-exitcode=1" \
	"${bindir}/${cmd}" ${args}
    set result = ${status}
    if ( "${result}" != "${expret}" ) then
	echo "Test program ${cmd} failed memory test".
	exit 1
    endif
else
    "${bindir}/${cmd}" ${args}
    set result = ${status}
    if ( "${result}" != "${expret}" ) then
	echo "Test program ${cmd} returned ${result}, while ${expret} was expected"
	exit 1
    endif
endif

exit 0
