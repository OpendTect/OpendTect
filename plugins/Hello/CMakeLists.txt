#_______________________Pmake___________________________________________________
#
#	Makefile : 	Hello plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-15 10:49:15 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Basic)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	hellopi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
