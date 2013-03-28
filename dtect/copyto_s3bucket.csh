#!/bin/csh

if ( $#argv < 2 ) then
        echo "Usage: $0 bucket source destination-directory ..."
        echo "f.i.: $0 opendtect /dsk/d22/test.txt classdoc ..."
	echo "if no third argument is given then it will transfer to the root of the bucket ..."
        exit 1
endif

if ( $#argv == 3 ) then
	set s3destinationdir=$3/
else if ( $#argv == 2 ) then
	set s3destinationdir=/
endif

set s3bucket=$1
set s3mounthost=dgb31
set s3mountdir=/mnt/s3/$1/
set rsyncoptions="--verbose --progress --compress --rsh=ssh --recursive --times --perms --owner --group --links"

ssh ${s3mounthost} "s3fs $1 -o allow_other ${s3mountdir}"
rsync ${rsyncoptions} $2 ${s3mounthost}:${s3mountdir}${s3destinationdir}
ssh ${s3mounthost} "sudo umount ${s3mountdir}"

