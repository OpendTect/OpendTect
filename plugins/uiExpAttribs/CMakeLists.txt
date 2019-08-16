#_______________________Pmake___________________________________________________
#
#	Makefile : 	Experimental Attributes
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS ExpAttribs uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uieventfreqattrib.cc
	uiexpattribspi.cc
	uigrubbsfilterattrib.cc
	uicorrmultiattrib.cc
	uisimilaritybyaw.cc
	uiintegratedtrace.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
