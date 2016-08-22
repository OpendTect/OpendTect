#_______________________Pmake___________________________________________________
#
#	Makefile : 	SEG-Y UI
# 	June 2015	Bert
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uimmptest.cc
	uimmptestpi.cc
)

set(OD_MODULE_BATCHPROGS od_mmptestbatch.cc )
set(OD_MODULE_PROGS od_MMPTestLaunch.cc )

set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
