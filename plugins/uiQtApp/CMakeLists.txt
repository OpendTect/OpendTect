#_______________________Pmake___________________________________________________
#
#	Makefile : 	Qt App plugin example
# 	March 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_USEQT Gui)
set(OD_MODULE_SOURCES
	qtclss.cc
	uiqtapppi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()

