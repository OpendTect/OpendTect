#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012-03-22 14:24:03 cvsbert Exp $
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
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
