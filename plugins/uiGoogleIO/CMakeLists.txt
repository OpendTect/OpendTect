#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiGoogleIO plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-15 10:49:16 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uigoogleexpsurv.cc
	uigoogleexpwells.cc
	uigoogleexp2dlines.cc
	uigoogleexppolygon.cc
	uigoogleexprandline.cc
	uigoogleiopi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
