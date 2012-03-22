#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiBouncy plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-03-22 09:07:59 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Bouncy uiODMain )
SET(OD_IS_PLUGIN yes)
SET(OD_USECOIN 1)
SET(OD_MODULE_SOURCES
	uibouncymain.cc
	uibouncysettingsdlg.cc
	visbouncydisplay.cc
	uibouncymgr.cc
	uibouncypi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
