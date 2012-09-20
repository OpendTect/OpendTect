#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiVoxelConnectivityFilter plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain VoxelConnectivityFilter)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uivoxelconnectivityfilterpi.cc
	uivoxelconnectivityfilter.cc )
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
