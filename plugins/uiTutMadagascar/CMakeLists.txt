#_______________________Pmake___________________________________________________
#
#	Makefile : 	TutMadagascar-related UI
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-15 10:49:16 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Seis uiFlatView uiODMain Madagascar)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uitutodmad.cc
	uitutodmadpi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
