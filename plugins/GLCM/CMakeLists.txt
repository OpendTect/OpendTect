#_______________________Pmake___________________________________________________
#
#	Makefile : 	GLCM plugin
# 	Jan 2012	C.G. Eichkitz
#	RCS :		$Id$
#_______________________________________________________________________________

#Setup the module
SET(OD_MODULE_NAME GLCM)
SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	GLCM_attrib.cc
	GLCM_pi.cc)
SET( OD_PLUGIN_EXECS ${OD_ATTRIB_EXECS} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
