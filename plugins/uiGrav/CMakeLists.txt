#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiGrav plugin
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_USECOIN 1)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	grav.cc
	gravhorcalc.cc
	uigravhorcalc.cc
	uigravpi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()

