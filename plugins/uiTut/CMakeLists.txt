#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-17 16:19:05 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_NAME uiTut)
SET(OD_MODULE_IS_PLUGIN)
SET(OD_MODULE_DEPS uiODMain Tut)
SET(OD_MODULE_SOURCES 
	uitutseistools.cc
	uituthortools.cc
	uitutorialattrib.cc
	uitutwelltools.cc
	uitutpi.cc)
OD_INIT_MODULE()
