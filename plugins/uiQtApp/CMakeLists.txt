#_______________________Pmake___________________________________________________
#
#	Makefile : 	Qt App plugin example
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-15 10:49:16 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_USEQT Gui)
SET(OD_MODULE_SOURCES
	qtclss.cc
	uiqtapppi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
