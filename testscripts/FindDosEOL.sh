#!/bin/sh
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#
# FindDosEOL.sh - finds files with DOS end-of-lines.
#

progname=$0

if [ $# -gt 1 ] && [ $1 == "--listfile" ]; then
  listfile=$2
  if [ ! -f $listfile ]; then
    echo "File $listfile does not exists."
  fi
elif [ $# -lt 1 ]; then
  echo "Usage: `basename $0` <listfile> [--wdir basedir]"
  echo
  echo ${progname} - Checks that all lines of all files end with a Unix-style new line character
  echo Returns 0 if all lines of all files end with a unix-style new line character, otherwise 1
  exit 1
fi

scriptdir=`dirname $0`

if [ -e "${scriptdir}/GetNrProc" ]; then
  nrcpus=`${scriptdir}/GetNrProc`
else
  nrcpus=8
fi

srcdir=`dirname $scriptdir`
if [ ${*: -2:1} == "--wdir" ] && [ -d "$BASH_ARGV" ]; then
  srcdir=$BASH_ARGV
fi

srcfiles=`cat $listfile | grep \\.ico -v | grep \\mod.h -v | grep \\Basic/buildinfo.h -v | grep \\odversion.h -v | grep _helpids.h -v | grep qpaths.cc -v | awk -v srcdir=${srcdir} '{print srcdir"/"$1;}'`
bindir=`dirname $listfile`
bindir=`dirname $bindir`
binfiles=`cd ${bindir}; cat $listfile | grep \\mod.h | awk -v dirnm=${bindir} '{print dirnm"/"$1;}'`
inputfiles=${srcfiles}" "${binfiles}

for onefile in ${inputfiles};
do
  if [ ! -f "${onefile}" ];then
    echo "File not found: $onefile"
    exit 1
  fi
done
files=`echo ${inputfiles} | xargs -P ${nrcpus} -n 200 grep -l $'\r'`

if [ ! -z "$files" ];then
  echo "DOS EOL found in: "
  echo $files
  exit 1
fi

exit ${status}

