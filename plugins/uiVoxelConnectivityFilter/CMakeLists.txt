#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiVoxelConnectivityFilter plugin
# 	Jan 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain VoxelConnectivityFilter)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uivoxelconnectivityfilterpi.cc
	uivoxelconnectivityfilter.cc )
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
