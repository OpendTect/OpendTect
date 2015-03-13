#_______________________Cmake___________________________________________________
#
#	Makefile : 	uiBasemap plugin
# 	Author:		Nanne Hemstra
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain Basemap )
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uibasemappi.cc
	uibasemapcontouritem.cc
	uibasemapgeom2ditem.cc
	uibasemapgriditem.cc
	uibasemaphorizon3ditem.cc
	uibasemapiomgr.cc
	uibasemapitem.cc
	uibasemappicksetitem.cc
	uibasemappolygonitem.cc
	uibasemaprandomlineitem.cc
	uibasemapscalebar.cc
	uibasemapseisoutlineitem.cc
	uibasemaptbmgr.cc
	uibasemapwellitem.cc
	uibasemapwin.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
