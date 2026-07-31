#!/usr/bin/env bash
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#
# Compresses PNGs using pngquant.
# Needs 'pngquant' installed.
#
# Input: A file containing PNG file names
#
# exit value: 0 = all OK, files replaced by compressed variant
# exit value non-zero: an error occurred
#

quiet=0
if [ "$1" = "-q" ] || [ "$1" = "--quiet" ]; then
    quiet=1
    shift
fi

if [ $# -ne 1 ]; then
    echo "Usage: $0 [-q|--quiet] file_with_png_file_names"
    exit 1
fi

if ! command -v pngquant >/dev/null 2>&1; then
    echo "pngquant not available, doing nothing"
    exit 2
fi

inpfile="$1"
if [ ! -f "$inpfile" ]; then
    echo "$inpfile does not exist"
    exit 1
fi

filesize()
{
    # Portable byte size (Linux and macOS)
    wc -c < "$1" | tr -d '[:space:]'
}

while IFS= read -r fnm || [ -n "$fnm" ]; do
    [ -z "$fnm" ] && continue
    if [ ! -w "$fnm" ]; then
	echo "$fnm is not writable. Correct this first"
	exit 3
    fi
done < "$inpfile"

before=0
after=0
totbefore=0
totafter=0

while IFS= read -r fnm || [ -n "$fnm" ]; do
    [ -z "$fnm" ] && continue

    fnmbase="${fnm%.png}"
    qfnm="${fnmbase}_quant.png"

    if [ "$quiet" -eq 0 ]; then
	printf '%s ' "$fnm"
    fi

    pngquant --ext _quant.png "$fnm" >/dev/null 2>&1

    if [ "$quiet" -eq 0 ]; then
	before=$( filesize "$fnm" )
	after=$( filesize "$qfnm" )
	totbefore=$(( totbefore + before ))
	totafter=$(( totafter + after ))
    fi

    mv -f "$qfnm" "$fnm"

    if [ "$quiet" -eq 0 ]; then
	if [ "$before" -gt 0 ]; then
	    perc=$(( (before - after) * 100 / before ))
	else
	    perc=0
	fi
	echo "${perc}%"
    fi
done < "$inpfile"

if [ "$quiet" -eq 0 ] && [ "$totbefore" -gt 0 ]; then
    totred=$(( totbefore - totafter ))
    perc=$(( totred * 100 / totbefore ))
    totred=$(( totred / 1024 ))
    totbefore=$(( totbefore / 1024 ))
    echo ""
    echo "Total reduction: $totred on $totbefore kB (${perc}%)"
fi

exit 0
