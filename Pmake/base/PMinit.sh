#---------------------------------------------------------
# Author: dGB Earth Sciences ( A.H.Bril )
# Pmake Environment bash initialization shell script
# $Id: PMinit.sh,v 1.2 2003-09-15 09:42:26 kristofer Exp $
#---------------------------------------------------------

if [ "$PMAKE" = "" ]; then
    if [ "$WORK" = "" ]; then
	PMAKE=/users/appman/pmake; export PMAKE
    else
	PMAKE=$WORK/Pmake/base; export PMAKE
    fi
fi

if [ "$GNUMAKE" = "" ]; then
    GNUMAKE="gmake"; export GNUMAKE
fi

PMAKECOMMAND='$GNUMAKE -I$WORK/Pmake -I$PMAKE'; export PMAKECOMMAND
alias make="$PMAKECOMMAND"

alias wdir='setenv WORK `pwd`'
alias cdw='cd $WORK'

_pmake_dbg_dir_=G
if [ "$DEBUG" = "no" ]; then
    _pmake_dbg_dir_=O
fi

alias cdb='cd $WORK/bin/$HDIR/'$_pmake_dbg_dir_
alias cdl='cd $WORK/lib/$HDIR/'$_pmake_dbg_dir_
