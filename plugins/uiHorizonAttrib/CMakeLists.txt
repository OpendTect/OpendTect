#_______________________Pmake___________________________________________________
#
#	Makefile : 	Horizon attributes and related
# 	March 2012	Nageswara
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain HorizonAttrib)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uicontourtreeitem.cc
	uidatapointsetpickdlg.cc
	uiflattenedcube.cc
	uihorizonattrib.cc
	uihorattribpi.cc
	uiisopachmaker.cc
	uistratamp.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
