#_______________________Pmake___________________________________________________
#
#	Makefile : 	DataPointSet demo
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uidpsdemopi.cc
	uidpsdemo.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
