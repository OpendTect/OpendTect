#_______________________Pmake___________________________________________________
#
#	Makefile : 	Madagascar (UI)
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain Madagascar)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uimadagascarmain.cc
	uimadbldcmd.cc
	uimadiosel.cc
	uimadpi.cc)

set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()


#ifdef MAD_UIMSG_IF_FAIL
#	OWNC++FLAGS := -DMAD_UIMSG_IF_FAIL
#endif
