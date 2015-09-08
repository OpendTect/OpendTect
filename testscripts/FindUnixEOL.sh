#!/bin/sh
#
# Copyright (C): dGB Beheer B. V.
#
# FindDosEOL.sh - finds files with DOS end-of-lines.
#

progname=$0

if [ $# -lt 1 ]
then
  echo "Usage: `basename $progname` [--listfile listfile] [files]"
  exit 1
fi

if [ "$1" = "--listfile" ]; then
    listfile=$2
    shift

    if [ ! -f $listfile ];
    then
       echo "File $listfile does not exists."
       exit 1
    fi

    cat $listfile | xargs $progname
    if [ "$?" -ne 0 ]; then
       exit 1
    fi

    shift
fi

res=0
while [ $# -gt 0 ]; do
    hexdump -v -e '"" 1/1 "%02X" " "' $1 | grep -q '[^0][^D] 0A'
    if [ "$?" -ne 1 ]; then
	echo "UNIX EOL found in $1"
	res=1
    fi
    shift
done

exit $res

