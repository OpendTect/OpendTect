#!/bin/csh
#
# Copyright (C): dGB Beheer B. V.
#
# $Id$
# 

set s3args=""
set verboseflag="-v"
set md5arg="--no-check-md5"
set progname = $0
set src=""
set dest=""

if ( $#argv<2 ) then
    goto syntax
endif

nextarg:

if ( "${1}" == "--bucket" ) then
    if ( $#argv<2 ) then
	echo "No arguments given to --bucket"
	goto syntax
    endif
    set bucket=${2}
    if ( "${bucket}" == "" ) then
        goto syntax
    endif
    shift
else if ( "${1}" == "--s3arg" ) then
    if ( $#argv<2 ) then
	echo "No arguments given to --s3arg"
	goto syntax
    endif
    set s3args="${s3args} '${2}'"
    shift
else if ( "${1}" == "--quiet" ) then
    set verboseflag=""
else if ( "${1}" == "--reduced-redundancy" ) then
    set s3args="${s3args} --rr"
else if ( "${1}" == "--md5" ) then
    set md5arg="--check-md5"
else if ( "${src}" == "" ) then
    set src=${1}
    if ( "${src}" == "" || ! -e ${src} ) then
	echo "Invalid source"
	goto syntax
    endif
else if ( "${dest}"=="" ) then
    set dest=${1}
    if ( "${dest}" == "" ) then
	echo "Invalid dest"
	goto syntax
    endif
endif

shift

if ( $#argv<1 ) then
    goto do_it
endif

goto nextarg


syntax:
    echo
    echo "Syntax: $progname <source> <destination> <--bucket bucket> [--reduced-redundancy] [--md5] [--s3arg arg] [--s3arg arg] [--quiet]"
    exit 1

do_it:

if ( "${src}" == "" ) then
    echo "No source specified"
    goto syntax
endif

if ( "${dest}" == "" ) then
    echo "No destintation specified. Assuming \"/\""
    goto syntax
endif

set s3cmd = "s3cmd sync ${s3args} ${verboseflag} ${md5arg} ${src} s3://${bucket}/${dest}/"
set s3host=dgb31

ssh -l dgb ${s3host} "${s3cmd}"
if ( ${status} > 0 ) then
    echo "The execution of \"${s3cmd}\" on ${s3host} failed"
    exit 1
endif

exit 0
