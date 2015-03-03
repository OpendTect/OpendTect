#!/bin/sh 

if [ "$#" -eq "3" ]; then
    for file in $2/data/localizations/source/*.ts ; do
	base=`basename ${file%.*}`
	$0 ${base} $1 $2 $3
    done
else
    if [ "$#" -eq "4" ]; then
	src=$3/data/localizations/source/$1.ts
	dst=$4/data/localizations/$1.qm
	
	if test $dst -ot $src; then
	    $2 $src -qm $dst
	fi
    else
	echo "Syntax: "
	echo "$0 <lrelease-exec> <srcdir> <destdir>"
    fi
fi

exit 0
