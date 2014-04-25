#!/bin/sh
#
# Copyright (C): dGB Beheer B. V.
#
# FindDosEOL.sh - finds help-ids in the help-id file and checks if they are present in the
#                 sources
#
#
#

if [ $# -le 1 ]
then
  echo "Usage: `basename $0` <headerfile> <source-basedir> [sourcebasedir1] ... [sourcebasedirN]"
  exit 1
fi

headerfile=$1

shift

searchpath=""
while [ $# -gt 0 ]
do
  basedir=$1
  searchpath="${searchpath} ${basedir}/src/*/*.cc ${basedir}/include/*/*.h ${basedir}/plugins/*/* ${basedir}/plugins/*/src/*/* ${basedir}/plugins/*/include/*/* ${basedir}/spec/*/*"

  shift

done

if [ ! -f $headerfile ];
then
   echo "File $headerfile does not exists."
   exit 1
fi

helpids=`egrep '^[[:space:]]*#[[:space:]]*define[[:space:]]+[A-Za-z0-9]+[[:space:]]+[A-Za-z0-9]+' ${headerfile} | awk '{print $2}'`

haserror=0
for helpid in ${helpids}
do
  count=`grep ${helpid} ${searchpath} 2> /dev/null | wc -l`
  if [ $count -lt 2 ]; then
      if [ ${haserror} -ne 1 ]; then
          echo "Missing HelpIDs:"
	  haserror=1
      fi
      echo " ${helpid}"
  fi
done

if [ ${haserror} -ne 1 ]; then
    exit 0
fi

exit 1

