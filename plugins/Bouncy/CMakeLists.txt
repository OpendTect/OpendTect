#_______________________Pmake___________________________________________________
#
#	Makefile : 	Bouncy plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-15 10:49:14 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	bouncycontroller.cc
	bouncypi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
