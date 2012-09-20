#_______________________Pmake___________________________________________________
#
#	Makefile : 	VoxelConnectivityFilter plugin
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS VolumeProcessing)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	voxelconnectivityfilterpi.cc
	voxelconnectivityfilter.cc )
SET( OD_PLUGIN_ALO_EXEC ${OD_VOLUME_EXECS} )
OD_INIT_MODULE()
