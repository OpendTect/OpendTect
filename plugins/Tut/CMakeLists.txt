#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-03-15 10:49:15 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine EarthModel Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	tutseistools.cc
	tuthortools.cc
	tutorialattrib.cc
	tutlogtools.cc
	tutpi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
