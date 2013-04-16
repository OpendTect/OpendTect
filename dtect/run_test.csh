#!/bin/csh -f
#
# Copyright (C): dGB Beheer B. V.
# $Id$
#
# Wrapper script to run a test
#

set datadir = ""
set wdir = ""
set plf = ""
set config = ""
set cmd = ""
set args = ""
set qtdir = ""

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
else if ( "$1" == "--quiet" ) then
    set args="${args} --quiet"
else if ( "$1" == "--wdir" ) then
    set wdir=$2
    shift
else if ( "$1" == "--config" ) then
    set config=$2
    shift
else if ( "$1" == "--parfile" ) then
    set args="${args} $2"
    shift
else if ( "$1" == "--qtdir" ) then
    set qtdir=$2
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

if ( $?LD_LIBRARY_PATH ) then
    setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:${qtdir}/lib:${OD_OSGDIR}/lib:${OD_COINDIR}/lib
else
    setenv LD_LIBRARY_PATH ${qtdir}/lib:${OD_OSGDIR}/lib:${OD_COINDIR}/lib
endif


if ( "$datadir" != "" ) then
    set args = "${args} --datadir=${datadir}"
endif

"${wdir}/bin/${plf}/${config}/${cmd}" ${args}
