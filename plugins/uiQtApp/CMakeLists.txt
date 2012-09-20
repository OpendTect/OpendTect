#_______________________Pmake___________________________________________________
#
#	Makefile : 	Qt App plugin example
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_USEQT Gui)
SET(OD_MODULE_SOURCES
	qtclss.cc
	uiqtapppi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
