#!/bin/csh -f
#
# Copyright (C): dGB Beheer B.V.
# Author: Bert
#
# Finds PNGs that will have significantly smaller size if compacted
# Needs 'pngquant' installed
#
# Input: a directory to search recursively downward for PNGs
#
# exit value: 0 = no files found (i.e. all good or an error)
#   - in that case, if there is something on stdout it must be an error
# exit value non-zero: number of compactable files found
#   - in that case, the file names will be put on std output
#
# $Id$


# --- Setup

# The cut-off percentage compression gain:
set cutoff_perc_gain=5

# --- Setup ends


if ( $#argv < 1 ) then
    echo "Usage: $0 directory_to_search_PNG_files"
    exit 0
endif

set topdir="$1"
if ( ! -e $topdir ) then
    echo "$topdir does not exist"
    exit 0
endif

which pngquant >& /dev/null
if ( $status != 0 ) then
    echo "pngquant not available, doing nothing"
    exit 0
endif


set nrfiles=0

foreach fil ( `find "$topdir" -name \*.png -print | sed -e 's/ /@SP@/g'` )

    set fnm=`echo $fil | sed 's/@SP@/ /g'`
    set fnmbase=`echo "$fnm" | sed 's/.png$//'`
    set qfnm="${fnmbase}_quant.png"

    set before=`ls -l "$fnm" | awk '{print $5}'`
    pngquant --ext _quant.png "$fnm" >& /dev/null
    if ( ! -e "$qfnm" ) continue

    set after=`ls -l "$qfnm" | awk '{print $5}'`
    rm -f "$qfnm"

    @ perc = $before - $after
    @ perc *= 100
    @ perc /= $before

    if ( $perc > $cutoff_perc_gain ) then
	@ nrfiles++
	echo "$fnm"
    endif

end

exit $nrfiles
