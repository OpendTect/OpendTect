#!/bin/csh
#
# Copyright (C): dGB Beheer B. V.
#
# $Id$
# 

if ( $#argv<1 ) then
    goto syntax
endif

set docdir=${1}

if ( "${docdir}" == "" ) then
    echo "No docdir specified"
    goto syntax
endif

if ( ! -e ${docdir} ) then
    echo "${docdir} does not exist"
    goto syntax
endif

cd ${docdir}
find . -name "*.html" | sed 's/\.\///g' | awk '{ print "http://static.opendtect.org/classdoc/html/"$1 }' > sitemap.txt

exit 0

syntax:
    echo "$0 <docdir>"
    exit 1
