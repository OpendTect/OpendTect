#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-03-21 15:58:14 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_NAME uiTut)
SET(OD_IS_PLUGIN 1 )
SET(OD_MODULE_DEPS uiODMain Tut)
SET(OD_MODULE_SOURCES 
	uitutseistools.cc
	uituthortools.cc
	uitutorialattrib.cc
	uitutwelltools.cc
	uitutpi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
