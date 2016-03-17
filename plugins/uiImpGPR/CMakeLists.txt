#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiImpGPR plugin
# 	Feb 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	dztimporter.cc
	uiimpgprpi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
