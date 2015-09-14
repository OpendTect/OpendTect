#_______________________Pmake___________________________________________________
#
#	Makefile : 	Horizon attributes and related
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain HorizonAttrib)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uicontourtreeitem.cc
	uidatapointsetpickdlg.cc
	uihorizonattrib.cc
	uihorattribpi.cc
	uiisopachmaker.cc
	uistratamp.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
