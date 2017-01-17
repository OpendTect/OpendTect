#!/bin/sh
#
# Copyright (C): dGB Beheer B. V.
#
# Downloads all dlsites logs, compresses them, and uploads compressed version,
# and deletes input logs from the Google storage bucket

if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` <gsutils>"
  exit 1
fi

gsutilcmd=$1

if [ ! -x "${gsutilcmd}" ]; then
    echo "${gsutilcmd} is not executable"
    exit 1
fi

#
# Create temporary directory
#
tmpdir="tmp$$"
mkdir ${tmpdir}
bucket=gs://opendtect-dlsites/archive


if [ ! -e "${tmpdir}" ]; then
    echo "${tmpdir} could not be created"
    exit 1
fi

#
# Sync all log files to temporary directory
#
${gsutilcmd} -m -q rsync -x ".*\.tar\.bz2" ${bucket} ${tmpdir}/

if [ ! $? -eq 0 ];then
   echo "Error: Failed to to sync folder ${tmpdir}"
   rm -rf ${tmpdir}
   exit 1
fi

#
# Create archive with all logs
#
cd ${tmpdir}
outputfile=`date +"dlsitestats-%Y-%m-%dT%H-%M-%S.tar.bz2"`
tar cfj ../${outputfile} .
if [ ! $? -eq 0 ];then
   echo "Error: Failed to create archive ${outputfile}"
   cd ..
   rm -rf ${tmpdir}
   exit 1
fi
cd ..

#
# Upload archive
#
${gsutilcmd} -q cp ${outputfile} ${bucket}

if [ ! $? -eq 0 ];then
   echo "Error: Failed to upload archive ${outputfile}"
   cd ..
   rm -rf ${tmpdir}
   exit 1
fi

# 
# Create list of files to remove files on server
#
rmfilelist="rmfiles$$"
rm -f ${rmfilelist}
if [ -e "${rmfilelist}" ]; then
    echo ${rmfilelist} cannot be removed.
    exit 1
fi
    
cd ${tmpdir}
for f in *; do
    remotefile="${bucket}/${f}"
    echo ${remotefile} >> ../${rmfilelist}
done
cd ..

exit
#
# Remove files on server
#
cat ${rmfilelist} | xargs ${gsutilcmd} -q -m rm 
if [ ! $? -eq 0 ];then
    echo "Error: Failed to remove files in ${remotefile} "
    echo "List of files to remove is in ${rmfilelist}"
    echo rm -rf ${tmpdir}

    exit 1
fi

#Cleanup
rm -f ${rmfilelist}
rm -rf ${tmpdir}

exit 0
