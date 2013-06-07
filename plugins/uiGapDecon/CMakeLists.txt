#_______________________Pmake___________________________________________________
#
#	Makefile : 	GapDecon-related UI
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012/03/29 11:33:25 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes uiFlatView GapDecon)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uigapdeconattrib.cc
	uigapdeconpi.cc
	uigdexamacorr.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
