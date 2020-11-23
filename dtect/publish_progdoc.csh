#!/bin/csh
#
# Copyright (C): dGB Beheer B. V.
#
# $Id$
# 

if ( $#argv<2 ) then
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

set serversubdir=progdoc/${1}
set docdir=${2}

if ( "${docdir}" == "" ) then
    echo "No docdir specified"
    goto syntax
endif

if ( ! -e ${docdir} ) then
    echo "${docdir} does not exist"
    goto syntax
endif

if ( ! -e ${docdir}/Generated/html ) then
    echo "${docdir}/Generated/html does not exist"
    exit 1
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

set files = `find . ! -path "*.svn*" ! -path "*.html.in"`

set listfile = ${docdir}/compfilelist
rm -rf ${listfile}

echo "Making Documentation directory structure"
foreach file ( ${files} )
    if ( "${file}" == "." ) then
	continue
    endif

    if ( -d ${file} ) then
	mkdir ${compdir}/${file}
    else
	echo ${file} >> ${listfile}
    endif
end

echo "Compressing Documentation"
cat ${listfile} | xargs -P8 ${dtectdir}/compress_doc.csh ${compdir}

cd ${prevdir}

#First, run to upload svg files, as they need special mime treatment
${sendappl} --s3arg --add-header --s3arg "Content-Encoding: gzip" --s3arg --exclude --s3arg "*" --s3arg --include --s3arg "*.svg" --s3arg -m --s3arg "image/svg+xml" ${compdir} ${serversubdir} --bucket static.opendtect.org --quiet --reduced-redundancy

#SecondlySecond, run to upload new stuff, keep removed files
${sendappl} --s3arg --add-header --s3arg "Content-Encoding: gzip" --s3arg --exclude --s3arg "*.svg" ${compdir} ${serversubdir} --bucket static.opendtect.org --quiet --reduced-redundancy

#Third, delete removed files
${sendappl} --s3arg --delete-removed --s3arg --add-header --s3arg "Content-Encoding: gzip" ${compdir} ${serversubdir} --bucket static.opendtect.org --quiet --reduced-redundancy
exit 0

syntax:
    echo "$0 <serversubdir> <docdir>"
    exit 1
