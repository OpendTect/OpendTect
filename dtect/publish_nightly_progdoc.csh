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

set prevdir = `pwd`
set dtectdir = `dirname ${progname}`
set sendappl = ${dtectdir}/copyto_s3bucket.csh
set nrcpus = `${dtectdir}/GetNrProc`
if ( ! -e ${sendappl} ) then
    echo "${sendappl} does not exist"
    exit 1
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
set compdir = "${docdir}/../compressed/"
if ( -e ${compdir} ) then
    rm -rf ${compdir}
endif

mkdir ${compdir}

find . -name "*.md5" | xargs -P ${nrcpus} rm -f
find . -name "*.map" | xargs -P ${nrcpus} rm -f
find . -name "*.tmp" | xargs -P ${nrcpus} rm -f
find . -name "*.tag" | xargs -P ${nrcpus} rm -f
find . -name "*.dot" | xargs -P ${nrcpus} rm -f
find . -name "*.dot" | xargs rm -f

set files = `find . ! -path "*.svn*"`

echo "Compressing Documentation"
foreach file ( ${files} )
    if ( "${file}" == "." ) then
	continue
    endif

    if ( -d ${file} ) then
	mkdir ${compdir}/${file}
    else
	gzip -c ${file} > ${compdir}/${file}
    endif
end

find . -name "*.html" | sed 's/\.\///g' | awk '{ print "http://static.opendtect.org/progdoc/"$1 }' | gzip > ${compdir}/sitemap.txt

cd ${prevdir}

#First, run to upload svg files, as they need special mime treatment
${sendappl} --s3arg --add-header --s3arg "Content-Encoding: gzip" --s3arg --exclude --s3arg "*" --s3arg --include --s3arg "*.svg" --s3arg -m --s3arg "image/svg+xml" ${compdir} progdoc --bucket static.opendtect.org --quiet --reduced-redundancy

#SecondlySecond, run to upload new stuff, keep removed files
${sendappl} --s3arg --add-header --s3arg --exclude --s3arg "*.svg" --s3arg "Content-Encoding: gzip" ${compdir} progdoc --bucket static.opendtect.org --quiet --reduced-redundancy

#Third, delete removed files
${sendappl} --s3arg --delete-removed --s3arg --add-header --s3arg "Content-Encoding: gzip" ${compdir} progdoc --bucket static.opendtect.org --quiet --reduced-redundancy
exit 0

syntax:
    echo "$0 <docdir>"
    exit 1
