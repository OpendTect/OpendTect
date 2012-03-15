#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiVoxelConnectivityFilter plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-15 10:49:17 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain VoxelConnectivityFilter)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uivoxelconnectivityfilterpi.cc
	uivoxelconnectivityfilter.cc )
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
