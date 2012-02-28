#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiVoxelConnectivityFilter plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-02-28 10:38:45 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain VoxelConnectivityFilter)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uivoxelconnectivityfilterpi.cc
	uivoxelconnectivityfilter.cc )
OD_INIT_MODULE()
