#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.5 2012-03-29 11:26:25 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine EarthModel Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	tutseistools.cc
	tuthortools.cc
	tutorialattrib.cc
	tutlogtools.cc
	tutpi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} ${OD_ATTRIB_EXECS} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
