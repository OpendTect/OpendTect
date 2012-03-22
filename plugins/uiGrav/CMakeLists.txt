#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiGrav plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-03-22 14:24:03 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_USECOIN 1)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	grav.cc
	gravhorcalc.cc
	uigravhorcalc.cc
	uigravpi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
