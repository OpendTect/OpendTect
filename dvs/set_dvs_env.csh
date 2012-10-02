#
# environent for dGB's CVS development tools
#SVN: $Id$

setenv DGB_CVS_HOST cvsserv
setenv OD_CVS_HOST cvs.opendtect.org

if ( ! $?DGB_CVS_DIR ) then
    setenv DGB_CVS_DIR /users/appman/dvs
    if ( $?WORK ) then
	if ( -e $WORK/dvs ) setenv DGB_CVS_DIR $WORK/dvs
    endif
endif

if ( ! $?OD_CVS_USER ) then
    echo "OD_CVS_USER environment not set, cvsanon used instead."
    setenv OD_CVSROOT :pserver:cvsanon@$OD_CVS_HOST\:/cvsroot
else
    setenv DGB_CVSROOT	:pserver:$USER@$DGB_CVS_HOST\:/cvsroot
    setenv OD_CVSROOT	:ext:$OD_CVS_USER@$OD_CVS_HOST\:/cvsroot
endif

setenv CVS_RSH ssh

if ( `hostname` == $DGB_CVS_HOST ) then
    echo "Do not work on $DGB_CVS_HOST\!"
    exit 1
endif

setenv CVSROOT $OD_CVSROOT

setenv PATH "${PATH}:${DGB_CVS_DIR}" 

# The CVSREAD=yes environmental variable forces the repository to be
# downloaded in read-only mode. This is required for the dvs_ed 
# commands, so you can become aware if others are already editing 
# a file you want to edit.
setenv CVSREAD yes
