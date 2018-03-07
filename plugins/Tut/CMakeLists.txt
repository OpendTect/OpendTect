#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	Bert
#_______________________________________________________________________________

set(OD_MODULE_DEPS AttributeEngine EarthModel VolumeProcessing Well)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	tutseistools.cc
	tuthortools.cc
	tutlogtools.cc
	tutorialattrib.cc
	tutvolproc.cc
	tutpi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} od_process_volume )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
