#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiGoogleIO plugin
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	uigoogleexpsurv.cc
	uigoogleexpwells.cc
	uigoogleexp2dlines.cc
	uigoogleexppolygon.cc
	uigoogleexprandline.cc
	uigoogleiopi.cc
	uigisexpdlg.cc)
    set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} ${OD_SURVEY_EXECS})
OD_INIT_MODULE()
