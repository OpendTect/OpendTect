#_______________________Pmake___________________________________________________
#
#	Makefile : 	GLCM plugin
# 	Jan 2012	C.G. Eichkitz
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_MODULE_NAME GLCM)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	GLCM_attrib.cc
	GLCM_pi.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
