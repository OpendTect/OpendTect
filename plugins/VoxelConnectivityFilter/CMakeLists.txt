#_______________________Pmake___________________________________________________
#
#	Makefile : 	VoxelConnectivityFilter plugin
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

set(OD_MODULE_DEPS VolumeProcessing)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	voxelconnectivityfilterpi.cc
	voxelconnectivityfilter.cc )
set( OD_PLUGIN_ALO_EXEC ${OD_VOLUME_EXECS} )
OD_INIT_MODULE()
