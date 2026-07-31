#!/usr/bin/env bash
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#
# Finds PNGs that will have significantly smaller size if compacted.
# Needs 'pngquant' installed.
#
# Input: one or more directories to search recursively downward for PNGs
#
# exit value: 0 = no files found (i.e. all good or an error)
#   - in that case, if there is something on stdout it must be an error
# exit value non-zero: number of compactable files found
#   - in that case, the file names will be put on std output
#

# The cut-off percentage compression gain:
cutoff_perc_gain=5

if [ $# -lt 1 ]; then
    echo "Usage: $0 directory_to_search_PNG_files [directory ...]"
    exit 0
fi

if ! command -v pngquant >/dev/null 2>&1; then
    echo "pngquant not available, doing nothing"
    exit 0
fi

filesize()
{
    # Portable byte size (Linux and macOS)
    wc -c < "$1" | tr -d '[:space:]'
}

nrfiles=0

for topdir in "$@"; do
    if [ ! -e "$topdir" ]; then
	echo "$topdir does not exist"
	continue
    fi

    while IFS= read -r -d '' fnm; do
	fnmbase="${fnm%.png}"
	qfnm="${fnmbase}_quant.png"

	before=$( filesize "$fnm" )
	pngquant --ext _quant.png "$fnm" >/dev/null 2>&1
	if [ ! -e "$qfnm" ]; then
	    continue
	fi

	after=$( filesize "$qfnm" )
	rm -f "$qfnm"

	if [ "$before" -le 0 ]; then
	    continue
	fi

	perc=$(( (before - after) * 100 / before ))
	if [ "$perc" -gt "$cutoff_perc_gain" ]; then
	    nrfiles=$(( nrfiles + 1 ))
	    echo "$fnm"
	fi
    done < <( find "$topdir" -name '*.png' -print0 )
done

exit "$nrfiles"
