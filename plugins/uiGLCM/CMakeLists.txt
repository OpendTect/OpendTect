#_______________________Pmake___________________________________________________
#
#	Makefile : 	GLCM plugin
# 	Jan 2012	C.G. Eichkitz
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_NAME uiGLCM)
SET(OD_IS_PLUGIN 1 )
SET(OD_MODULE_DEPS uiODMain GLCM)
SET(OD_MODULE_SOURCES 
	uiGLCM_attrib.cc
	uiGLCM_pi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
SET( OD_NO_ALO_ENTRY 1 )
OD_INIT_MODULE()
