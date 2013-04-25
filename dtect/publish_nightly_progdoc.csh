#!/bin/csh
#
# Copyright (C): dGB Beheer B. V.
#
# $Id$
# 

if ( $#argv<1 ) then
    goto syntax
endif

set progname=${0}

set dtectdir = `dirname ${progname}`
set sendappl = ${dtectdir}/copyto_s3bucket.csh
if ( ! -e ${sendappl} ) then
    echo "${sendappl} does not exist"
    exit 1
endif

set prevdir = `pwd`

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
set compdir = "${docdir}/../compressed/"
if ( -e ${compdir} ) then
    rm -rf ${compdir}
endif

mkdir ${compdir}

find . -name "*.md5" | xargs rm -f
find . -name "*.map" | xargs rm -f
find . -name "*.tmp" | xargs rm -f
find . -name "*.tag" | xargs rm -f

echo "Compressing Documentation"
foreach file ( `find .` )
    if ( "${file}" == "." ) then
	continue
    endif

    if ( "${file}" == ".svn" ) then
	continue
    endif

    if ( -d $file ) then
	mkdir ${compdir}${file}
    else
	gzip -c ${file} > ${compdir}${file}
    endif
end

find . -name "*.html" | sed 's/\.\///g' | awk '{ print "http://static.opendtect.org/progdoc/"$1 }' | gzip > ${compdir}/sitemap.txt

cd ${prevdir}

${sendappl} --s3arg --add-header --s3arg "Content-Encoding: gzip" ${compdir} progdoc --bucket static.opendtect.org --quiet
exit 0

syntax:
    echo "$0 <docdir>"
    exit 1
