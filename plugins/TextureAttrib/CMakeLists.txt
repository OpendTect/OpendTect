#_______________________Pmake___________________________________________________
#
#	Makefile : 	Texture plugin (base)
# 	Sep 2012	Paul
#	RCS :		$Id$
#_______________________________________________________________________________

#Setup the module
SET(OD_MODULE_NAME Texture)
SET(OD_MODULE_DEPS AttributeEngine EarthModel Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	textureattrib.cc
	textureattribpi.cc)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
