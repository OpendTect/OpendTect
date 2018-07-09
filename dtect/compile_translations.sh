#!/bin/sh 

if [ "$#" -eq "3" ]; then
    for file in $2/data/translations/*.ts ; do
	base=`basename ${file} .ts`
	$0 ${base} $1 $2 $3
    done
else
    if [ "$#" -eq "4" ]; then
	src=$3/data/translations/$1.ts
	dst=$4/data/translations/$1.qm
	
	#Ensure data directory exists
        if [ ! -e "$4/data/" ]; then
            mkdir $4/data
        else
            if [ ! -d $4/data ]; then
                echo "$4/data exists and is not a directory."
                exit 1;
            fi
        fi

	#Ensure data/translations directory exists
	if [ ! -e "$4/data/translations/" ]; then
	    mkdir $4/data/translations
	else
	    if [ ! -d $4/data/translations ]; then
		echo "$4/data/translations exists and is not a directory."
		exit 1;
	    fi
	fi
	
	if test $dst -ot $src; then
	    $2 $src -qm $dst
	fi
    else
	echo "Syntax: "
	echo "$0 <lrelease-exec> <srcdir> <destdir>"
    fi
fi

exit 0

