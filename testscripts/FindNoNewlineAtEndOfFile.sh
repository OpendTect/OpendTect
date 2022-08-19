#!/bin/sh 
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#
# FindNoNewlineAtEndOfFile.sh - finds missing EOL character at the file end
#

progname=$0

if [ $# -gt 1 ] && [ $1 == "--listfile" ]; then
  listfile=$2
  if [ ! -f $listfiles ]; then
    echo "File $listfile does not exists."
    exit 1
  fi
elif [ $# -lt 1 ]; then
  echo "Usage: ${progname} <--listfile <listfile> [--wdir basedir] | files ..>"
  echo
  echo ${progname} - Checks that all files end with a new line
  echo Returns 0 if all files end with a newline, otherwise 1
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

if [ ! -z "${listfile+xxx}" ]; then
  if [ ! -f "${listfile}" ]; then
    echo "File ${listfile} does not exists."
    exit 1
  fi

  srcfiles=`cat $listfile | grep \\.ico -v | grep \\mod.h -v | grep \\Basic/buildinfo.h -v | grep \\odversion.h -v | grep _helpids.h -v | grep qpaths.cc -v | awk -v srcdir=${srcdir} '{print srcdir"/"$1;}'`
  bindir=`dirname $listfile`
  bindir=`dirname $bindir`
  binfiles=`cd ${bindir}; cat $listfile | grep \\mod.h | awk -v dirnm=${bindir} '{print dirnm"/"$1;}'`
  inputfiles=${srcfiles}" "${binfiles}
    
  for onefile in ${inputfiles};
  do
    if [ ! -f ${onefile} ]; then
      echo "File not found: $onefile"
      exit 1
    fi
  done
  files=`echo ${inputfiles} | xargs -P ${nrcpus} -n 200 ${progname}`
  if [ ! -z "${files}" ];then
    echo "Missing EOL at end of file found in: "
    echo $files
  fi
else
  for onefile in "$@"
  do
    if [ ! -f ${onefile} ]; then
      echo "File ${onefile} does not exist."
    fi
    res=`tail -c1 ${onefile} | /usr/bin/od -x | awk -v filenm=${onefile} '{if (NR==1 && $2!="000a") print filenm;}'`
    if [ ! -z ${res} ]; then
      echo "${onefile}"
      exit 1
    fi
  done
  exit 0
fi

exit ${status}
