#_______________________Pmake___________________________________________________
#
#	Makefile : 	Tutorial plugin
# 	Jan 2012	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS AttributeEngine EarthModel Well)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	tutseistools.cc
	tuthortools.cc
	tutorialattrib.cc
	tutlogtools.cc
	tutpi.cc)
set( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
set( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
