#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_NAME uiTut)
set(OD_IS_PLUGIN 1 )
set(OD_MODULE_DEPS uiODMain Tut)
set(OD_MODULE_SOURCES
	uitutseistools.cc
	uituthortools.cc
	uitutorialattrib.cc
	uitutwelltools.cc
	uitutpi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()

