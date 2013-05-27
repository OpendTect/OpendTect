#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define installer variables
# Author:       Nageswara
# Date:         May 2013
#RCS:           $Id$

set( LIBLIST Algo Basic General Network uiBase uiODInstMgr uiTools uiCmdDriver )
set( THIRDPARTYLIBS QtCore QtGui QtNetwork QtOpenGL )
set( EXECLIST  od_instmgr )
if( WIN32 )
    set( EXECLIST ${EXECLIST} od_setup )
else()
    set( ODSCRIPTS run_installer )
endif()

set( DATAFILES contexthelp.png proxysettings.png )
