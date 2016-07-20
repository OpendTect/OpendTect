#!/bin/sh
#
# Copyright (C): dGB Beheer B. V.
#
# Downloads all dlsites logs, compresses them, and uploads compressed version,
# and deletes input logs from the S3 bucket

if [ $# -ne 3 ]
then
  echo "Usage: `basename $0` <s3cmd> <access key> <secret key>"
  exit 1
fi

s3cmd=$1
accesskey=$2
secretkey=$3

if [ ! -x "${s3cmd}" ]; then
    echo "${s3cmd} is not executable"
    exit 1
fi

#
# Create temporary directory
#
tmpdir="tmp$$"
mkdir ${tmpdir}

if [ ! -e "${tmpdir}" ]; then
    echo "${tmpdir} could not be created"
    exit 1
fi

#
# Sync all log files to temporary directory
#
${s3cmd} --quiet \
	--access_key=${accesskey} \
	--secret_key=${secretkey} \
	sync \
	--exclude '*.tar.bz2' \
	s3://dlsites/archive/ \
	${tmpdir}/

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
${s3cmd} --quiet \
	--storage-class=STANDARD_IA \
	--region=eu-west-1 \
	--access_key=${accesskey} \
	--secret_key=${secretkey} \
	put ${outputfile} s3://dlsites/archive/

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
    remotefile="s3://dlsites/archive/${f}"
    echo ${remotefile} >> ../${rmfilelist}
done
cd ..

#
# Remove files on server
#
cat ${rmfilelist} | xargs ${s3cmd} --quiet --access_key=${accesskey} --secret_key=${secretkey} rm 
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
