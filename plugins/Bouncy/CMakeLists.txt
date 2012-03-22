#_______________________Pmake___________________________________________________
#
#	Makefile : 	Bouncy plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-03-22 09:07:59 cvskris Exp $
#_______________________________________________________________________________

SET( OD_MODULE_DEPS uiODMain)
SET( OD_IS_PLUGIN yes)
SET( OD_MODULE_SOURCES
	bouncycontroller.cc
	bouncypi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
