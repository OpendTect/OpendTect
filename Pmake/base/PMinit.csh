#
#---------------------------------------------------------
# Author: de Groot - Bril Earth Sciences ( A.H.Bril )
# Pmake Environment initialization C-shell script
# $Id: PMinit.csh,v 1.5 2003-10-29 09:53:42 arend Exp $
#---------------------------------------------------------

if ( ! $?PMAKE ) then
    if ( $?WORK ) then
	setenv PMAKE $WORK/Pmake/base
    else
	setenv PMAKE /users/appman/pmake
    endif
endif

if ( ! $?GNUMAKE ) then
    if ( $HDIR == win ) then
#	setenv GNUMAKE	"$PMAKE/bin/win/gmake-wrap"
	setenv GNUMAKE	"/usr/bin/make"
    else
	setenv GNUMAKE	"gmake"
    endif
endif

setenv PMAKECOMMAND	'$GNUMAKE -I$WORK/Pmake -I$PMAKE'
alias make		$PMAKECOMMAND

alias wdir		'setenv WORK `pwd`'
alias cdw		'cd $WORK'

set _pmake_dbg_dir_=G
if ( $?DEBUG ) then
    if ( $DEBUG == no ) set _pmake_dbg_dir_=O
else
    set _pmake_dbg_dir_=O
endif

alias cdb		'cd $WORK/bin/$HDIR/'$_pmake_dbg_dir_
alias cdl		'cd $WORK/lib/$HDIR/'$_pmake_dbg_dir_

unset _pmake_dbg_dir_
