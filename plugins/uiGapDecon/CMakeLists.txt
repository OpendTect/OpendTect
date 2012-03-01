#_______________________Pmake___________________________________________________
#
#	Makefile : 	GapDecon-related UI
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 11:31:28 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiAttributes uiFlatView GapDecon)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uigapdeconattrib.cc
	uigapdeconpi.cc
	uigdexamacorr.cc)
OD_INIT_MODULE()
