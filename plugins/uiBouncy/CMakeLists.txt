#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiBouncy plugin
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS Bouncy uiODMain )
set(OD_IS_PLUGIN yes)
set(OD_USECOIN 1)
set(OD_MODULE_SOURCES
	uibouncymain.cc
	uibouncysettingsdlg.cc
	visbouncydisplay.cc
	uibouncymgr.cc
	uibouncypi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()

