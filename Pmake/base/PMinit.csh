#
#---------------------------------------------------------
# Author: de Groot - Bril Earth Sciences ( A.H.Bril )
# Pmake Environment initialization C-shell script
# $Id: PMinit.csh,v 1.13 2010/12/15 15:49:32 cvsbert Exp $
#---------------------------------------------------------

if ( ! $?HDIR ) then
    
    setenv HDIR `uname`
    if ( "$HDIR" == "SunOS" ) then
	setenv HDIR sun5
    else if ( "$HDIR" == "Linux" ) then
	setenv HDIR lux
    else if ( "$HDIR" == "Darwin" ) then
	setenv HDIR mac
    else if ( $?COMSPEC ) then
	setenv HDIR win
    endif

endif

if ( ! $?PLFSUBDIR ) then
    setenv PLFSUBDIR $HDIR
    if ( $HDIR == sun5 ) then
	setenv PLFSUBDIR sol32
    else if ( $HDIR == lux ) then
	if ( `uname -a | grep _64 | wc -l` == 1 ) then
	    setenv PLFSUBDIR lux64
	else
	    setenv PLFSUBDIR lux32
	endif
    endif
endif

if ( ! $?PMAKE ) then
    if ( $?WORK ) then
	setenv PMAKE $WORK/Pmake/base
    else
	setenv PMAKE /d3/dgb/Pmake/base
    endif
endif

if ( ! $?GNUMAKE ) then
    if ( $HDIR == win ) then
	setenv GNUMAKE	"/usr/bin/make"
    else if ( $HDIR == mac ) then
	setenv GNUMAKE "/usr/bin/gnumake"
    else
	setenv GNUMAKE	"gmake"
    endif
endif

setenv PMAKECOMMAND	'$GNUMAKE --no-print-directory -I$WORK/Pmake -I$PMAKE'
alias make		$PMAKECOMMAND

alias wdir		'setenv WORK `pwd`'
alias wdir		'setenv WORK `pwd`;setenv PMAKE $WORK/Pmake/base; setenv OD_WORKDIR `$PMAKE/bin/print_od_workdir`'
alias cdw		'cd $WORK'

setenv DEBUG_SUBDIR G
if ( $?DEBUG ) then
    if ( $DEBUG == no ) then
	setenv DEBUG_SUBDIR O
    else if ( $DEBUG == optyes ) then
	setenv DEBUG_SUBDIR OG
    endif
else
    setenv DEBUG_SUBDIR O
endif

alias cdb		'cd $WORK/bin/$PLFSUBDIR/'$DEBUG_SUBDIR
alias cdl		'cd $WORK/lib/$PLFSUBDIR/'$DEBUG_SUBDIR
