#!/bin/sh
#
# Copyright (C): dGB Beheer B. V.
#
# create_mac_icon.sh - Takes a png-file and creates a
# multi-resolution mac icon
#
# $Id$
#

if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` <pngfile>"
  exit 1
fi

pngfile=$1

if [ ! -f $pngfile ];
then
   echo "File $pngfile does not exists."
   exit 1
fi

pngfiledir=`dirname $1`
pngfilebasename=`basename $1`

sipsexec=`which sips`
if [ ! -x $sipsexec ];
then
    echo "Cannot find sips executable"
    exit 1
fi

iconutilexec=`which iconutil`
if [ ! -x $iconutilexec ];
then
    echo "Cannot find iconutil executable"
    exit 1
fi

tmpdir=${pngfilebasename}.iconset
if [ -d $tmpdir ];
then
    echo "$tmpdir exists. Please remove and re-run"
    exit 1
fi

mkdir ${tmpdir}

if [ ! -d $tmpdir ];
then
   echo "Cannot create $tmpdir"
fi

$sipsexec -z 32 32 ${pngfile} --out ${tmpdir}/icon_32x32.png
$sipsexec -z 64 64 ${pngfile} --out ${tmpdir}/icon_32x32@2x.png
$sipsexec -z 64 64 ${pngfile} --out ${tmpdir}/icon_64x64.png
$sipsexec -z 128 128 ${pngfile} --out ${tmpdir}/icon_64x64@2x.png
$sipsexec -z 128 128 ${pngfile} --out ${tmpdir}/icon_128x128.png
$sipsexec -z 256 256 ${pngfile} --out ${tmpdir}/icon_128x128@2x.png
$sipsexec -z 256 256 ${pngfile} --out ${tmpdir}/icon_256x256.png
$sipsexec -z 512 512 ${pngfile} --out ${tmpdir}/icon_256x256@2x.png
$sipsexec -z 512 512 ${pngfile} --out ${tmpdir}/icon_512x512.png

$iconutilexec -c icns ${tmpdir}
if [ ! $? -eq 0 ];then
   echo "Failed to create icon"
   exit 1
fi

rm -rf ${tmpdir}

echo "Icon created"
exit 0



