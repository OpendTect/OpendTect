#_______________________________________________________________________________
#
# (C) dGB Beheer B.V.
# $Id: .init.bash,v 1.1 2010-04-06 08:33:09 relman Exp $
#
# Script to setup Pmake environment. Source from .bashrc
#
#_______________________________________________________________________________

export WORK="__WORK__"
export DTECT_APPL="__DTECTAPPL__DIR__"

#export OD_QTDIR=""
#export OD_COINDIR=""

if [ ${DEBUG:-'No'} == "No" ]; then
    export DEBUG=yes
fi


if [ ${GNUMAKE:-'No'} == "No" ]; then
    makeloc=`which make`

    if [ -x $makeloc ]; then
	makever=`$makeloc --version | grep GNU`
    fi

    if [ ! "$makever" = "" ]; then
	export GNUMAKE=$makeloc
    fi
fi

if [ ${GNUMAKE:-'No'} == "No" ]; then
    echo "Could not find GNU make. please install it and set GNUMAKE"
fi

export PMAKE=
. "$WORK/Pmake/base/PMinit.sh"
