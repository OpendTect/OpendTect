#_______________________________________________________________________________
#
# (C) dGB Beheer B.V.
# $Id: .init.csh,v 1.1 2010-04-06 08:33:09 relman Exp $
#
# Script to setup Pmake environment. Source from .login
#
#_______________________________________________________________________________

setenv WORK "__WORK__"
setenv DTECT_APPL "__DTECTAPPL__DIR__"

#setenv OD_QTDIR
#setenv OD_COINDIR

if ( ! $?DEBUG ) then
    setenv DEBUG yes
endif


if ( ! $?GNUMAKE ) then
    set makeloc=`which make`

    if ( -x $makeloc ) then
	set makever=`$makeloc --version | grep GNU`
    endif

    if ( "$makever" != "" ) then
	setenv GNUMAKE "$makeloc"
    endif
endif

if ( ! $?GNUMAKE ) then
    echo "Could not find GNU make. please install it and set GNUMAKE"
endif

unsetenv PMAKE
source "$WORK/Pmake/base/PMinit.csh"
