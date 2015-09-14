#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiGoogleIO plugin
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uigoogleexpsurv.cc
	uigoogleexpwells.cc
	uigoogleexp2dlines.cc
	uigoogleexppolygon.cc
	uigoogleexprandline.cc
	uigoogleiopi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
