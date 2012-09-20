#_______________________Pmake___________________________________________________
#
#	Makefile : 	GapDecon-related UI
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes uiFlatView GapDecon)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uigapdeconattrib.cc
	uigapdeconpi.cc
	uigdexamacorr.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
