#_______________________Pmake___________________________________________________
#
#	Makefile : 	GapDecon plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-09-17 12:49:54 cvsjaap Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiCmdDriver uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	cmddriverpi.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
