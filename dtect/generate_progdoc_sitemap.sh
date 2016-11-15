#!/bin/bash
#
# Copyright (C): dGB Beheer B. V.
#

if [ $# -ne 2 ]; then
    goto syntax
fi

docdir=${1}
prefix=${2}

cd ${docdir}

#Create sitemap
awkprog=/tmp/awkcmd_$$.awk
echo '{ print "'"${prefix}"'/"$1 }' > $awkprog

find . -name "*.html" | sed 's/\.\///g' | awk -f ${awkprog} > sitemap.txt

rm -rf ${awkprog}

exit 0

syntax:
    echo "$0 <docdir> <urlprefix>"
    exit 1

