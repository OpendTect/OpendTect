#!/bin/csh -f
#
# Copyright (C): dGB Beheer B. V.
# $Id$
#
# Wrapper script to run a test
#

set datadir = ""
set parfile = ""

parse_args:
if ( "$1" == "--command" ) then
    set cmd=$2
    shift
if ( "$1" == "--datadir" ) then
    set datadir=$2
    shift
if ( "$1" == "--parfile" ) then
    set parfile=$2
    shift
else if ( "$1" == "--help" ) then
    echo "Usage: run_tst [option]"
    echo " --command cmd"
    echo " --ldpath ldpath"
    echo " --datadir datadir"
    echo " [--parfile parfile]"
    exit 0
else
    goto do_it
endif

shift
goto parse_args

do_it:

if ( $?LD_LIBRARY_PATH ) then
    setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${OD_QTDIR}/lib:${OD_OSGDIR}/lib:${OD_COINDIR}/lib
else
    setenv LD_LIBRARY_PATH ${OD_QTDIR}/lib:${OD_OSGDIR}/lib:${OD_COINDIR}/lib
endif

if ( "$datadir" != "" ) then
    set cmd = "${cmd} --datadir=${datadir}"
endif

if ( "$parfile" != "" ) then
    set cmd = "${cmd} ${parfile}"
endif

${cmd}
