#!/bin/sh
#
# Copyright (C): dGB Beheer B. V.
#
# FindDosEOL.sh - finds files with DOS end-of-lines.
#

if [ $# -lt 1 ]
then
  echo "Usage: `basename $0` <listfile> [--wdir basedir]"
  exit 1
fi

listfile=$1

if [ ! -f $listfile ];
then
   echo "File $listfile does not exists."
   exit 1
fi

scriptdir=`dirname $0`

if [ -e "${scriptdir}/GetNrProc" ]; then
    nrcpus=`${scriptdir}/GetNrProc`
else
    nrcpus=8
fi

srcdir=`dirname $scriptdir`
if [ $# == 3 ];
then
   srcdir=$3
fi

bindir=`dirname $listfile`
bindir=`dirname $bindir`

cd $srcdir

srcfiles=`cat $listfile | grep \\.ico -v | grep \\mod.h -v`
binfiles=`cat $listfile | grep \\mod.h`

for onefile in $srcfiles;
   do
      onefile="$srcdir/$onefile"
      if [ ! -e "$onefile" ];then
         echo "File not found: $onefile"
         exit 1
      fi
done

for onefile in $binfiles;
   do
      onefile="$bindir/$onefile"
      if [ ! -e "$onefile" ];then
         echo "File not found: $onefile"
         exit 1
      else
	 binfiles="$binfiles $onefile"
      fi
done

allfiles="$srcfiles $binfiles"

files=`echo $allfiles | xargs -P ${nrcpus} -n 200 grep -l $'\r'`
if [ -z "$files" ];then
   echo "No DOS EOL found!"
else
   echo "DOS EOL found in: "
   echo $files
   exit 1
fi

