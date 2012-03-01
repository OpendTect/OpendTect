#_______________________Pmake___________________________________________________
#
#	Makefile : 	TutMadagascar-related UI
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 12:51:17 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Seis uiFlatView uiODMain Madagascar)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uitutodmad.cc
	uitutodmadpi.cc)
OD_INIT_MODULE()
