#!/usr/bin/env bash
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#
# Finds the presence of a keyword in files, unless they are listed
# (as a path substring) in an exception file.
#
# Returns 0 if keyword is not found, otherwise 1
#

export LANG=C
export LC_CTYPE=C

progname="$0"
exceptionfile=""
listfile=""
keyword=""
message="Keyword found"
grepcommand=grep

usage()
{
    echo
    echo "${progname} - Finds the presence of keyword in files, unless they are"
    echo "present in an exceptionfile."
    echo
    echo "Returns 0 if keyword is not found, otherwise 1"
    echo
    echo "Syntax ${progname} --keyword <kw> [--message <msg>] [--exceptionfile <file>] <--listfile <listfile> | files ..>"
    echo
    exit 1
}

if [ $# -lt 2 ]; then
    usage
fi

while [ $# -gt 0 ]; do
    case "$1" in
	--keyword)
	    shift
	    keyword="$1"
	    ;;
	--exceptionfile)
	    exceptionfile="$2"
	    if [ -z "${exceptionfile}" ]; then
		usage
	    fi
	    if [ ! -e "${exceptionfile}" ]; then
		echo "Exception file \"${exceptionfile}\" does not exist."
		exit 1
	    fi
	    shift
	    ;;
	--listfile)
	    listfile="$2"
	    if [ -z "${listfile}" ]; then
		usage
	    fi
	    if [ ! -e "${listfile}" ]; then
		echo "List-file \"${listfile}\" does not exist."
		exit 1
	    fi
	    shift
	    ;;
	--message)
	    message="$2"
	    if [ -z "${message}" ]; then
		usage
	    fi
	    shift
	    ;;
	--grepcommand)
	    grepcommand="$2"
	    if [ -z "${grepcommand}" ]; then
		usage
	    fi
	    shift
	    ;;
	*)
	    break
	    ;;
    esac
    shift
done

if [ -n "${listfile}" ]; then
    if [ -n "${exceptionfile}" ]; then
	# shellcheck disable=SC2002
	cat "${listfile}" | xargs -P 0 -n 200 \
	    "${progname}" --keyword "${keyword}" \
	    --exceptionfile "${exceptionfile}" \
	    --message "${message}" \
	    --grepcommand "${grepcommand}"
	exit $?
    else
	# shellcheck disable=SC2002
	cat "${listfile}" | xargs -P 0 -n 200 \
	    "${progname}" --keyword "${keyword}" \
	    --message "${message}" \
	    --grepcommand "${grepcommand}"
	exit $?
    fi
fi

failure=0

for filename in "$@"; do
    [ -z "${filename}" ] && continue
    [ ! -e "${filename}" ] && continue

    if cat "${filename}" | \
	# Remove // comments
	sed 's/\/\/.*//' | \
	# Make everything one line
	sed '{:q;N;s/\n/ /g;t q}' | \
	# Remove /* */ comments
	sed 's/\/\*.*\*\///' | \
	# Search for keyword
	${grepcommand} -q -- "${keyword}"
    then
	if [ -n "${exceptionfile}" ]; then
	    # Match if any exception entry is a substring of the filename
	    if echo "${filename}" | grep -Fq -f "${exceptionfile}"; then
		continue
	    fi
	fi

	echo "${filename}"
	failure=1
    fi
done

if [ "${failure}" -eq 1 ]; then
    echo "${message}"
fi

exit "${failure}"
