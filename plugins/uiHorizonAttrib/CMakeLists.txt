#_______________________Pmake___________________________________________________
#
#	Makefile : 	Horizon attributes and related
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain HorizonAttrib)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uihorizonattrib.cc
	uihorattribpi.cc
	uistratamp.cc
	uiflattenedcube.cc
	uiisopachmaker.cc
	uicontourtreeitem.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
