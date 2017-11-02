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

cd $srcdir
srcfiles=`cat $listfile | grep \\.ico -v | grep \\mod.h -v | grep \\Basic/buildinfo.h -v | grep \\odversion.h -v | grep _helpids.h -v`
for onefile in $srcfiles;
   do
      onefile="$srcdir/$onefile"
      if [ ! -e "$onefile" ];then
         echo "File not found: $onefile"
         exit 1
      fi
done
files=`echo $srcfiles | xargs -P ${nrcpus} -n 200 grep -l $'\r'`
allfiles=$files

bindir=`dirname $listfile`
bindir=`dirname $bindir`
cd $bindir
binfiles=`cat $listfile | grep \\mod.h`
for onefile in $binfiles;
   do
      onefile="$bindir/$onefile"
      if [ ! -e "$onefile" ];then
         echo "File not found: $onefile"
         exit 1
      fi
done
files=`echo $binfiles | xargs -P ${nrcpus} -n 200 grep -l $'\r'`
if [ -z "$allfiles" ];then
   allfiles="$files"
else
   allfiles="$allfiles $files"
fi

if [ -z "$allfiles" ];then
   echo "No DOS EOL found!"
else
   echo "DOS EOL found in: "
   echo $allfiles
   exit 1
fi

