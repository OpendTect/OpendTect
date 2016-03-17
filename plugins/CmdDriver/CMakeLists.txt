#_______________________Pmake___________________________________________________
#
#	Makefile : 	GapDecon plugin
# 	Feb 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiCmdDriver uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	#coincommands.cc
	cmddriverpi.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
