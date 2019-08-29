#!/bin/bash

if [ ! -z "$1" ]; then

    if [ -x "$(command -v $1)" ]; then
	echo $1
	exit 0
    fi
fi

declare -a candidates=( "konsole" "gnome-terminal" "terminator" "guake" "yakuake" "tilda" "macterm" "xterm" )

for cand in "${candidates[@]}"
do
    if [ -x "$(command -v $cand)" ]; then
	echo $cand
	exit 0
    fi
done

echo "xterm"
exit 1
