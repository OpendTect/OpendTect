#!/bin/sh
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

if [ $# -lt 4 ]; then
    echo "$0"
    echo " tries to resolve a minidump file using symbols in a symbol"
    echo " directory, save it as a text file, and optionally"
    echo " launch a sender applicaiton with the text-file as argument."
    echo ""   
    echo " The dump-file will be deleted"   
    echo ""   
    echo -n "Usage : $0 <dumpfile> <symbol-dir> <resolve application> "
    echo    "<archive-prefix> [sender] [args to sender]"
    exit 1
fi

#Extract the input parameters
dumpfile=${1}
shift

symboldir=${1}
shift

dumphandler=${1}
shift

archivename=${1}
shift

sender=""
if [ $# -gt 0 ]; then
    sender=${1}
    shift

    if [ ! -x "${sender}" ]; then
	echo ${sender} is not an executable
	exit 1
    fi
fi

#Check the input parameters
if [ ! -e "${dumpfile}" ]; then
    echo ${dumpfile} does not exist
    exit 1
fi

if [ ! -d "${symboldir}" ]; then
    echo ${symboldir} is not a directory
    exit 1
fi

if [ ! -x "${dumphandler}" ]; then
    echo ${dumphandler} is not an executable
    exit 1
fi

tmpfile=${dumpfile}_$$.txt
tmpdir=`dirname ${tmpfile}`
archivefile=${tmpdir}/${archivename}_${USER}.txt

if [ -e ${tmpfile} ]; then
    rm ${tmpfile}

    if [ -e ${tmpfile} ]; then
	echo "${tmpfile} exists, and I cannot remove it."
	exit 1
    fi
fi

if [ -e ${tmpfile} ] && [ ! -w ${tmpfile} ]; then
    echo "Cannot write to ${tmpfile}"
    exit 1
fi

#Create the human-readable text-file
${dumphandler} ${dumpfile} ${symboldir} > ${tmpfile} 2> /dev/null

#Create machine-readable text
echo "Machine readable:" >> ${tmpfile}
${dumphandler} -m ${dumpfile} ${symboldir} >> ${tmpfile} 2> /dev/null

#Send the text-file
if [ "${sender}" != "" ] && [ -x ${sender} ] && [ -e ${tmpfile} ]; then
    ${sender} ${tmpfile} $*
fi

#Make an archive copy of the report without timestamps so it can be picked up
#Timestamps will only fill disks
cp ${tmpfile} ${archivefile}
if [ -e ${archivefile} ]; then
    echo "Error report saved as ${archivefile}"
fi

#Cleanup
rm ${dumpfile}
rm ${tmpfile}
