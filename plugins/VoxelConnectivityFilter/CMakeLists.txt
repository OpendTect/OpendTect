#_______________________Pmake___________________________________________________
#
#	Makefile : 	VoxelConnectivityFilter plugin
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-15 10:49:17 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS VolumeProcessing)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	voxelconnectivityfilterpi.cc
	voxelconnectivityfilter.cc )
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} ${OD_VOLUME_EXECS} )
OD_INIT_MODULE()
