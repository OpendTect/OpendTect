#_______________________Pmake___________________________________________________
#
#	Makefile : 	Non-Linear Analysis Ui
# 	Sep 2003	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

add_definitions( -DmNoAutoUiString )

set(OD_MODULE_DEPS uiIo NLA Well Seis )
set(OD_MODULE_SOURCES uinlapartserv.cc )
OD_INIT_MODULE()

