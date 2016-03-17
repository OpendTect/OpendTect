#_______________________Pmake___________________________________________________
#
#	Makefile : 	TutMadagascar-related UI
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS Seis uiFlatView uiODMain Madagascar)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uitutodmad.cc
	uitutodmadpi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
