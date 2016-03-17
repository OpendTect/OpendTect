#_______________________Pmake___________________________________________________
#
#	Makefile : 	GLCM plugin
# 	Jan 2012	C.G. Eichkitz
#_______________________________________________________________________________

set(OD_MODULE_DEPS AttributeEngine)
set(OD_MODULE_NAME GLCM)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	GLCM_attrib.cc
	GLCM_pi.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
