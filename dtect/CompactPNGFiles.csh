#!/bin/csh -f
#
# Copyright (C): dGB Beheer B.V.
# Author: Bert
#
# Compresses PNGs using pngquant.
# Needs 'pngquant' installed
#
# Input: A file containing PNG file names
#
# exit value: 0 = all OK, files replaced by compressed variant
# exit value non-zero: an error occurred
#
# $Id$

set quiet=0
if ( "$1" == "-q" || "$1" == "--quiet" ) then
    set quiet=1
    shift
endif

if ( $#argv != 1 ) then
    echo "Usage: $0 file_with_png_file_names"
    exit 1
endif

which pngquant >& /dev/null
if ( $status != 0 ) then
    echo "pngquant not available, doing nothing"
    exit 2
endif


set inpfile="$1"

foreach fil ( `cat "$inpfile" |sed 's/ /@SP@/g'` )

    set fnm=`echo $fil | sed 's/@SP@/ /g'`
    if ( ! -w "$fnm" ) then
	echo "$fnm is not writable. Correct this first"
	exit 3
    endif
end


set before=0
set after=0
set totbefore=0
set totafter=0

foreach fil ( `cat "$inpfile" |sed 's/ /@SP@/g'` )

    set fnm=`echo $fil | sed 's/@SP@/ /g'`
    set fnmbase=`echo "$fnm" | sed 's/.png$//'`
    set qfnm="${fnmbase}_quant.png"

    if ( $quiet == 0 ) echo -n "$fnm "
    pngquant --ext _quant.png "$fnm" >& /dev/null

    if ( $quiet == 0 ) then
	set before=`ls -l "$fnm" | awk '{print $5}'`
	set after=`ls -l "$qfnm" | awk '{print $5}'`
	@ totbefore += $before
	@ totafter += $after
    endif

    mv -f "$qfnm" "$fnm"

    if ( $quiet == 0 ) then
	@ perc = $before - $after
	@ perc *= 100
	@ perc /= $before
	echo "${perc}%"
    endif

end

if ( $quiet == 0 && $totbefore > 0 ) then
    @ totred = $totbefore - $totafter
    @ perc = ($totred * 100) / $totbefore
    @ totred /= 1024
    @ totbefore /= 1024
    echo ""
    echo "Total reduction: $totred on $totbefore kB (${perc}%)"
endif

exit 0
